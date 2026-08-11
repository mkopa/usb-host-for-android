#include "transport/session.hpp"

#include <algorithm>
#include <exception>
#include <new>
#include <utility>
#include <vector>

namespace usbhost::transport {

namespace {

void applyGeneration(DeviceDescriptor &device, SnapshotGeneration generation) {
    device.generation = generation;
    for (auto &configuration : device.configurations) {
        configuration.generation = generation;
        for (auto &interfaceDescriptor : configuration.interfaces) {
            interfaceDescriptor.generation = generation;
            for (auto &alternate : interfaceDescriptor.alternateSettings) {
                alternate.generation = generation;
                for (auto &endpoint : alternate.endpoints) {
                    endpoint.generation = generation;
                }
            }
        }
    }
}

}  // namespace

std::shared_ptr<TransportSession> TransportSession::open(
        int borrowedFileDescriptor,
        AuthorizedBackendFactory &factory,
        TransportError &outError) {
    outError = {};
    if (borrowedFileDescriptor < 0) {
        outError = makeTransportError(BackendStatus::InvalidArgument,
                                      "authorized file descriptor is invalid");
        return nullptr;
    }

    std::unique_ptr<UsbBackend> backend;
    std::string diagnostic;
    BackendStatus status = BackendStatus::InternalFailure;
    try {
        status = factory.openAuthorizedFileDescriptor(
            borrowedFileDescriptor, backend, diagnostic);
    } catch (const std::exception &) {
        diagnostic = "backend factory failed";
    }
    if (status != BackendStatus::Success || !backend) {
        if (backend) {
            backend->close();
        }
        if (status == BackendStatus::Success) {
            status = BackendStatus::InternalFailure;
            diagnostic = "backend factory returned no backend";
        }
        outError = makeTransportError(status, diagnostic);
        return nullptr;
    }

    try {
        DeviceDescriptor descriptor = backend->deviceDescriptor();
        auto session = std::shared_ptr<TransportSession>(
            new TransportSession(std::move(backend), std::move(descriptor)));
        outError = makeTransportError(BackendStatus::Success, {});
        return session;
    } catch (const std::bad_alloc &) {
        if (backend) {
            backend->close();
        }
        outError = makeTransportError(BackendStatus::InternalFailure,
                                      "session allocation failed");
        return nullptr;
    }
}

TransportSession::TransportSession(std::unique_ptr<UsbBackend> backend,
                                   DeviceDescriptor descriptor)
    : backend_(std::move(backend)), descriptor_(std::move(descriptor)),
      state_(SessionState::Open) {}

TransportSession::~TransportSession() {
    close();
}

usbhost_status TransportSession::close() {
    std::unique_ptr<UsbBackend> backend;
    std::vector<std::uint8_t> claimedInterfaces;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (state_ == SessionState::Closed) {
            return USBHOST_OK;
        }
        if (state_ == SessionState::Closing) {
            closedCondition_.wait(lock, [this] { return state_ == SessionState::Closed; });
            return USBHOST_OK;
        }
        state_ = SessionState::Closing;
        claimedInterfaces.reserve(claims_.size());
        for (const auto &claim : claims_) {
            claimedInterfaces.push_back(claim.first);
        }
        claims_.clear();
        backend = std::move(backend_);
    }

    if (backend) {
        for (const std::uint8_t interfaceNumber : claimedInterfaces) {
            backend->releaseInterface(interfaceNumber);
        }
        backend->close();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = SessionState::Closed;
    }
    closedCondition_.notify_all();
    return USBHOST_OK;
}

TransportError TransportSession::selectConfiguration(std::uint8_t configurationValue) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::Open || !backend_) {
        return {USBHOST_INVALID_STATE, 0, "session is not open"};
    }
    if (!claims_.empty()) {
        return makeTransportError(BackendStatus::Busy,
                                  "interfaces must be released before configuration selection");
    }
    const auto requested = std::find_if(
        descriptor_.configurations.begin(), descriptor_.configurations.end(),
        [configurationValue](const ConfigurationDescriptor &configuration) {
            return configuration.configurationValue == configurationValue;
        });
    if (requested == descriptor_.configurations.end()) {
        return makeTransportError(BackendStatus::InvalidArgument,
                                  "configuration value is unavailable");
    }

    const BackendStatus status = backend_->selectConfiguration(configurationValue);
    if (status != BackendStatus::Success) {
        return makeTransportError(status, "configuration selection failed");
    }

    DeviceDescriptor refreshed = backend_->deviceDescriptor();
    const auto selected = std::find_if(
        refreshed.configurations.begin(), refreshed.configurations.end(),
        [configurationValue](const ConfigurationDescriptor &configuration) {
            return configuration.configurationValue == configurationValue;
        });
    if (selected == refreshed.configurations.end()) {
        state_ = SessionState::Failed;
        return makeTransportError(BackendStatus::InternalFailure,
                                  "backend snapshot omitted selected configuration");
    }
    for (auto &configuration : refreshed.configurations) {
        configuration.active = configuration.configurationValue == configurationValue;
    }
    applyGeneration(refreshed, descriptor_.generation.next());
    descriptor_ = std::move(refreshed);
    return makeTransportError(BackendStatus::Success, {});
}

TransportError TransportSession::claimInterface(std::uint8_t interfaceNumber,
                                                InterfaceClaimToken &outToken) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::Open || !backend_) {
        return {USBHOST_INVALID_STATE, 0, "session is not open"};
    }
    const auto activeConfiguration = std::find_if(
        descriptor_.configurations.begin(), descriptor_.configurations.end(),
        [](const ConfigurationDescriptor &configuration) { return configuration.active; });
    if (activeConfiguration == descriptor_.configurations.end()) {
        return {USBHOST_INVALID_STATE, 0, "session has no active configuration"};
    }
    const auto interfaceIterator = std::find_if(
        activeConfiguration->interfaces.begin(), activeConfiguration->interfaces.end(),
        [interfaceNumber](const InterfaceDescriptor &interfaceDescriptor) {
            return interfaceDescriptor.interfaceNumber == interfaceNumber;
        });
    if (interfaceIterator == activeConfiguration->interfaces.end()) {
        return makeTransportError(BackendStatus::InvalidArgument, "interface is unavailable");
    }
    if (claims_.find(interfaceNumber) != claims_.end()) {
        return makeTransportError(BackendStatus::Busy, "interface is already claimed");
    }
    const BackendStatus status = backend_->claimInterface(interfaceNumber);
    if (status != BackendStatus::Success) {
        return makeTransportError(status, "interface claim failed");
    }

    InterfaceClaimToken token{interfaceNumber, nextClaimGeneration_, descriptor_.generation};
    ++nextClaimGeneration_;
    if (nextClaimGeneration_ == 0) {
        nextClaimGeneration_ = 1;
    }
    claims_.emplace(interfaceNumber, token);
    interfaceIterator->claimed = true;
    outToken = token;
    return makeTransportError(BackendStatus::Success, {});
}

TransportError TransportSession::selectAlternateSetting(InterfaceClaimToken &token,
                                                        std::uint8_t alternateSetting) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::Open || !backend_) {
        return {USBHOST_INVALID_STATE, 0, "session is not open"};
    }
    const auto claim = claims_.find(token.interfaceNumber);
    if (claim == claims_.end() || claim->second.claimGeneration != token.claimGeneration ||
        claim->second.snapshotGeneration != token.snapshotGeneration) {
        return {USBHOST_INVALID_STATE, 0, "interface claim token is stale"};
    }
    const auto activeConfiguration = std::find_if(
        descriptor_.configurations.begin(), descriptor_.configurations.end(),
        [](const ConfigurationDescriptor &configuration) { return configuration.active; });
    if (activeConfiguration == descriptor_.configurations.end()) {
        return {USBHOST_INVALID_STATE, 0, "session has no active configuration"};
    }
    const auto interfaceIterator = std::find_if(
        activeConfiguration->interfaces.begin(), activeConfiguration->interfaces.end(),
        [&token](const InterfaceDescriptor &value) {
            return value.interfaceNumber == token.interfaceNumber;
        });
    if (interfaceIterator == activeConfiguration->interfaces.end()) {
        return {USBHOST_INVALID_STATE, 0, "claimed interface is absent"};
    }
    const bool available = std::any_of(
        interfaceIterator->alternateSettings.begin(), interfaceIterator->alternateSettings.end(),
        [alternateSetting](const AlternateSettingDescriptor &value) {
            return value.alternateSetting == alternateSetting;
        });
    if (!available) {
        return makeTransportError(BackendStatus::InvalidArgument,
                                  "alternate setting is unavailable");
    }
    const BackendStatus status = backend_->selectAlternateSetting(
        token.interfaceNumber, alternateSetting);
    if (status != BackendStatus::Success) {
        return makeTransportError(status, "alternate setting selection failed");
    }

    DeviceDescriptor refreshed = backend_->deviceDescriptor();
    const std::uint8_t activeValue = activeConfiguration->configurationValue;
    auto refreshedConfiguration = std::find_if(
        refreshed.configurations.begin(), refreshed.configurations.end(),
        [activeValue](const ConfigurationDescriptor &value) {
            return value.configurationValue == activeValue;
        });
    if (refreshedConfiguration == refreshed.configurations.end()) {
        state_ = SessionState::Failed;
        return makeTransportError(BackendStatus::InternalFailure,
                                  "backend snapshot omitted active configuration");
    }
    for (auto &configuration : refreshed.configurations) {
        configuration.active = configuration.configurationValue == activeValue;
    }
    auto refreshedInterface = std::find_if(
        refreshedConfiguration->interfaces.begin(), refreshedConfiguration->interfaces.end(),
        [&token](const InterfaceDescriptor &value) {
            return value.interfaceNumber == token.interfaceNumber;
        });
    if (refreshedInterface == refreshedConfiguration->interfaces.end()) {
        state_ = SessionState::Failed;
        return makeTransportError(BackendStatus::InternalFailure,
                                  "backend snapshot omitted claimed interface");
    }
    refreshedInterface->activeAlternateSetting = alternateSetting;
    const SnapshotGeneration generation = descriptor_.generation.next();
    applyGeneration(refreshed, generation);
    for (auto &configuration : refreshed.configurations) {
        for (auto &interfaceDescriptor : configuration.interfaces) {
            interfaceDescriptor.claimed = claims_.find(interfaceDescriptor.interfaceNumber)
                != claims_.end();
        }
    }
    descriptor_ = std::move(refreshed);
    for (auto &storedClaim : claims_) {
        storedClaim.second.snapshotGeneration = generation;
    }
    token.snapshotGeneration = generation;
    return makeTransportError(BackendStatus::Success, {});
}

TransportError TransportSession::releaseInterface(const InterfaceClaimToken &token) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::Open || !backend_) {
        return {USBHOST_INVALID_STATE, 0, "session is not open"};
    }
    const auto claim = claims_.find(token.interfaceNumber);
    if (claim == claims_.end() || claim->second.claimGeneration != token.claimGeneration ||
        claim->second.snapshotGeneration != token.snapshotGeneration) {
        return {USBHOST_INVALID_STATE, 0, "interface claim token is stale"};
    }
    const BackendStatus status = backend_->releaseInterface(token.interfaceNumber);
    if (status != BackendStatus::Success) {
        return makeTransportError(status, "interface release failed");
    }
    claims_.erase(claim);
    for (auto &configuration : descriptor_.configurations) {
        for (auto &interfaceDescriptor : configuration.interfaces) {
            if (interfaceDescriptor.interfaceNumber == token.interfaceNumber) {
                interfaceDescriptor.claimed = false;
            }
        }
    }
    return makeTransportError(BackendStatus::Success, {});
}

TransportError TransportSession::validateEndpoint(const InterfaceClaimToken &token,
                                                  const EndpointDescriptor &endpoint) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::Open) {
        return {USBHOST_INVALID_STATE, 0, "session is not open"};
    }
    const auto claim = claims_.find(token.interfaceNumber);
    if (claim == claims_.end() || claim->second.claimGeneration != token.claimGeneration ||
        claim->second.snapshotGeneration != token.snapshotGeneration ||
        endpoint.generation != descriptor_.generation ||
        endpoint.interfaceNumber != token.interfaceNumber) {
        return {USBHOST_INVALID_STATE, 0, "endpoint or claim token is stale"};
    }
    for (const auto &configuration : descriptor_.configurations) {
        if (!configuration.active) continue;
        for (const auto &interfaceDescriptor : configuration.interfaces) {
            if (interfaceDescriptor.interfaceNumber != token.interfaceNumber ||
                !interfaceDescriptor.claimed) continue;
            for (const auto &alternate : interfaceDescriptor.alternateSettings) {
                if (alternate.alternateSetting != interfaceDescriptor.activeAlternateSetting) {
                    continue;
                }
                const auto found = std::find_if(
                    alternate.endpoints.begin(), alternate.endpoints.end(),
                    [&endpoint](const EndpointDescriptor &candidate) {
                        return candidate.address == endpoint.address &&
                            candidate.direction == endpoint.direction &&
                            candidate.transferType == endpoint.transferType &&
                            candidate.generation == endpoint.generation;
                    });
                if (found != alternate.endpoints.end()) {
                    return makeTransportError(BackendStatus::Success, {});
                }
            }
        }
    }
    return {USBHOST_INVALID_STATE, 0, "endpoint is not active for the claim"};
}

SessionState TransportSession::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

DeviceDescriptor TransportSession::descriptorSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return descriptor_;
}

}  // namespace usbhost::transport

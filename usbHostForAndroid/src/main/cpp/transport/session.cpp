#include "transport/session.hpp"

#include <algorithm>
#include <exception>
#include <new>
#include <utility>

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
        backend = std::move(backend_);
    }

    if (backend) {
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

SessionState TransportSession::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

DeviceDescriptor TransportSession::descriptorSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return descriptor_;
}

}  // namespace usbhost::transport

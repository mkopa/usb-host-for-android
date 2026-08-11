#include "usbhost/transport.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "android/android_usb_backend.hpp"
#include "core/c_api_state.hpp"
#include "transport/error.hpp"
#include "transport/registry.hpp"
#include "transport/session.hpp"

namespace {

using usbhost::transport::DeviceDescriptor;
using usbhost::transport::InterfaceClaimToken;
using usbhost::transport::TransportError;
using usbhost::transport::TransportSession;

std::mutex cApiStateMutex;
std::unordered_set<usbhost_transport_session> retiredHandles;
std::unordered_map<usbhost_transport_session,
                   std::unordered_map<std::uint8_t, InterfaceClaimToken>> claimTokens;

#if defined(USBHOST_BUILD_TESTS)
int fixtureOpenStatus = USBHOST_OK;
int fixtureObservedFd = -1;
#endif

usbhost_status finish(usbhost_status status, std::string_view diagnostic = {}) {
    return usbhost::detail::setLastResult(status,
        usbhost::transport::sanitizeDiagnostic(diagnostic));
}

usbhost_status finish(const TransportError &error) {
    return finish(error.status, error.diagnostic);
}

usbhost_status invalid(const char *diagnostic) {
    return finish(USBHOST_INVALID_ARGUMENT, diagnostic);
}

usbhost_status internalFailure() {
    return finish(USBHOST_INTERNAL_ERROR, "unexpected transport C API failure");
}

std::shared_ptr<TransportSession> findSession(usbhost_transport_session handle) {
    if (handle == USBHOST_TRANSPORT_INVALID_SESSION) return nullptr;
    return std::dynamic_pointer_cast<TransportSession>(
        usbhost::transport::globalTransportRegistry().find(handle));
}

template <typename Record>
bool validRecord(const Record *record) {
    return record != nullptr && record->struct_size >= sizeof(Record);
}

const usbhost::transport::ConfigurationDescriptor *configurationAt(
        const DeviceDescriptor &device, std::uint32_t index) {
    return index < device.configurations.size() ? &device.configurations[index] : nullptr;
}

const usbhost::transport::InterfaceDescriptor *interfaceAt(
        const DeviceDescriptor &device, std::uint32_t configurationIndex,
        std::uint32_t interfaceIndex) {
    const auto *configuration = configurationAt(device, configurationIndex);
    return configuration && interfaceIndex < configuration->interfaces.size()
        ? &configuration->interfaces[interfaceIndex] : nullptr;
}

const usbhost::transport::AlternateSettingDescriptor *alternateAt(
        const DeviceDescriptor &device, std::uint32_t configurationIndex,
        std::uint32_t interfaceIndex, std::uint32_t alternateIndex) {
    const auto *interfaceDescriptor = interfaceAt(device, configurationIndex, interfaceIndex);
    return interfaceDescriptor && alternateIndex < interfaceDescriptor->alternateSettings.size()
        ? &interfaceDescriptor->alternateSettings[alternateIndex] : nullptr;
}

const usbhost::transport::EndpointDescriptor *endpointAt(
        const DeviceDescriptor &device, std::uint32_t configurationIndex,
        std::uint32_t interfaceIndex, std::uint32_t alternateIndex,
        std::uint32_t endpointIndex) {
    const auto *alternate = alternateAt(
        device, configurationIndex, interfaceIndex, alternateIndex);
    return alternate && endpointIndex < alternate->endpoints.size()
        ? &alternate->endpoints[endpointIndex] : nullptr;
}

#if defined(USBHOST_BUILD_TESTS)
struct FixtureBackendState { int closeCalls{0}; };

class FixtureBackend final : public usbhost::transport::UsbBackend {
public:
    explicit FixtureBackend(std::shared_ptr<FixtureBackendState> state)
        : state_(std::move(state)) {
        using namespace usbhost::transport;
        descriptor_.usbVersionBcd = 0x0210;
        descriptor_.vendorId = 0x1234;
        descriptor_.productId = 0x5678;
        descriptor_.generation = SnapshotGeneration::initial();
        InterfaceDescriptor interfaceDescriptor;
        interfaceDescriptor.interfaceNumber = 3;
        interfaceDescriptor.activeAlternateSetting = 0;
        interfaceDescriptor.generation = descriptor_.generation;
        for (std::uint8_t alternateNumber = 0; alternateNumber < 2; ++alternateNumber) {
            AlternateSettingDescriptor alternate;
            alternate.interfaceNumber = 3;
            alternate.alternateSetting = alternateNumber;
            alternate.generation = descriptor_.generation;
            alternate.additionalDescriptors.push_back({0x24, {3, 0x24, 0xaa}});
            EndpointDescriptor endpoint;
            endpoint.address = static_cast<std::uint8_t>(0x81 + alternateNumber);
            endpoint.number = static_cast<std::uint8_t>(1 + alternateNumber);
            endpoint.direction = Direction::In;
            endpoint.transferType = TransferType::Bulk;
            endpoint.maximumPacketSize = 64;
            endpoint.interfaceNumber = 3;
            endpoint.alternateSetting = alternateNumber;
            endpoint.generation = descriptor_.generation;
            alternate.endpoints.push_back(endpoint);
            interfaceDescriptor.alternateSettings.push_back(alternate);
        }
        ConfigurationDescriptor configuration;
        configuration.configurationValue = 1;
        configuration.active = true;
        configuration.generation = descriptor_.generation;
        configuration.interfaces.push_back(interfaceDescriptor);
        descriptor_.configurations.push_back(configuration);
    }
    const DeviceDescriptor &deviceDescriptor() const noexcept override { return descriptor_; }
    usbhost::transport::BackendStatus selectConfiguration(std::uint8_t value) override {
        for (auto &configuration : descriptor_.configurations)
            configuration.active = configuration.configurationValue == value;
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus claimInterface(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus selectAlternateSetting(
            std::uint8_t, std::uint8_t alternate) override {
        descriptor_.configurations[0].interfaces[0].activeAlternateSetting = alternate;
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus releaseInterface(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::OperationId submitControl(
            const usbhost::transport::ControlRequest &, usbhost::transport::MutableBufferView,
            usbhost::transport::CompletionCallback) override {
        return usbhost::transport::kInvalidOperationId;
    }
    usbhost::transport::OperationId submitEndpoint(
            const usbhost::transport::EndpointTransferRequest &,
            usbhost::transport::MutableBufferView,
            usbhost::transport::CompletionCallback) override {
        return usbhost::transport::kInvalidOperationId;
    }
    bool cancel(usbhost::transport::OperationId) override { return false; }
    void close() noexcept override {
        if (!closed_) { closed_ = true; ++state_->closeCalls; }
    }
private:
    std::shared_ptr<FixtureBackendState> state_;
    DeviceDescriptor descriptor_;
    bool closed_{false};
};

class FixtureFactory final : public usbhost::transport::AuthorizedBackendFactory {
public:
    usbhost::transport::BackendStatus openAuthorizedFileDescriptor(
            int fd, std::unique_ptr<usbhost::transport::UsbBackend> &backend,
            std::string &diagnostic) override {
        fixtureObservedFd = fd;
        if (fixtureOpenStatus != USBHOST_OK) {
            diagnostic = "fixture open failed";
            return static_cast<usbhost::transport::BackendStatus>(
                fixtureOpenStatus == USBHOST_PERMISSION_DENIED
                    ? static_cast<int>(usbhost::transport::BackendStatus::PermissionDenied)
                    : static_cast<int>(usbhost::transport::BackendStatus::InternalFailure));
        }
        state = std::make_shared<FixtureBackendState>();
        backend = std::make_unique<FixtureBackend>(state);
        return usbhost::transport::BackendStatus::Success;
    }
    std::shared_ptr<FixtureBackendState> state;
};

FixtureFactory fixtureFactory;
#endif

usbhost::transport::AuthorizedBackendFactory &activeFactory() {
#if defined(USBHOST_BUILD_TESTS)
    return fixtureFactory;
#else
    static usbhost::android::AndroidUsbBackendFactory factory;
    return factory;
#endif
}

}  // namespace

extern "C" {

usbhost_status usbhost_transport_open_fd(
        int authorized_fd, usbhost_transport_session *out_session) {
    if (out_session == nullptr || authorized_fd < 0) return invalid("invalid open arguments");
    *out_session = USBHOST_TRANSPORT_INVALID_SESSION;
    try {
        TransportError error;
        auto session = TransportSession::open(authorized_fd, activeFactory(), error);
        if (!session) return finish(error);
        const auto handle = usbhost::transport::globalTransportRegistry().insert(session);
        if (handle == USBHOST_TRANSPORT_INVALID_SESSION) {
            session->close();
            return finish(USBHOST_INTERNAL_ERROR, "transport handle allocation failed");
        }
        *out_session = handle;
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_close(usbhost_transport_session handle) {
    if (handle == USBHOST_TRANSPORT_INVALID_SESSION) return finish(USBHOST_OK);
    try {
        auto retired = std::dynamic_pointer_cast<TransportSession>(
            usbhost::transport::globalTransportRegistry().retire(handle));
        if (!retired) {
            std::lock_guard<std::mutex> lock(cApiStateMutex);
            return retiredHandles.count(handle) ? finish(USBHOST_OK)
                                                : finish(USBHOST_INVALID_STATE, "unknown session");
        }
        {
            std::lock_guard<std::mutex> lock(cApiStateMutex);
            retiredHandles.insert(handle);
            claimTokens.erase(handle);
        }
        return finish(retired->close(), {});
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_device_descriptor(
        usbhost_transport_session handle,
        usbhost_transport_device_descriptor *out) {
    if (!validRecord(out)) return invalid("invalid device descriptor output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto value = session->descriptorSnapshot();
        usbhost_transport_device_descriptor result{};
        result.struct_size = sizeof(result);
        result.snapshot_generation = value.generation.value();
        result.usb_version_bcd = value.usbVersionBcd;
        result.device_class = value.deviceClass;
        result.device_subclass = value.deviceSubclass;
        result.device_protocol = value.deviceProtocol;
        result.endpoint_zero_max_packet_size = value.endpointZeroMaximumPacketSize;
        result.vendor_id = value.vendorId;
        result.product_id = value.productId;
        result.device_release_bcd = value.deviceReleaseBcd;
        result.configuration_count = static_cast<std::uint32_t>(value.configurations.size());
        *out = result;
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_configuration_count(
        usbhost_transport_session handle, std::uint32_t *out_count) {
    if (!out_count) return invalid("invalid configuration count output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        *out_count = static_cast<std::uint32_t>(
            session->descriptorSnapshot().configurations.size());
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_configuration_at(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        usbhost_transport_configuration_descriptor *out) {
    if (!validRecord(out)) return invalid("invalid configuration output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = configurationAt(snapshot, configuration_index);
        if (!value) return invalid("configuration index is out of range");
        usbhost_transport_configuration_descriptor result{};
        result.struct_size = sizeof(result); result.configuration_index = configuration_index;
        result.snapshot_generation = value->generation.value();
        result.configuration_value = value->configurationValue;
        result.attributes = value->attributes; result.maximum_power = value->maximumPower;
        result.active = value->active ? 1 : 0;
        result.interface_count = static_cast<std::uint32_t>(value->interfaces.size());
        result.additional_descriptor_count =
            static_cast<std::uint32_t>(value->additionalDescriptors.size());
        *out = result; return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_interface_count(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t *out_count) {
    if (!out_count) return invalid("invalid interface count output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *configuration = configurationAt(snapshot, configuration_index);
        if (!configuration) return invalid("configuration index is out of range");
        *out_count = static_cast<std::uint32_t>(configuration->interfaces.size());
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_interface_at(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t interface_index, usbhost_transport_interface_descriptor *out) {
    if (!validRecord(out)) return invalid("invalid interface output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = interfaceAt(snapshot, configuration_index, interface_index);
        if (!value) return invalid("interface index is out of range");
        usbhost_transport_interface_descriptor result{};
        result.struct_size = sizeof(result); result.interface_index = interface_index;
        result.snapshot_generation = value->generation.value();
        result.interface_number = value->interfaceNumber;
        result.active_alternate_setting = value->activeAlternateSetting;
        result.claimed = value->claimed ? 1 : 0;
        result.alternate_setting_count =
            static_cast<std::uint32_t>(value->alternateSettings.size());
        *out = result; return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_alternate_setting_count(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t interface_index, std::uint32_t *out_count) {
    if (!out_count) return invalid("invalid alternate setting count output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = interfaceAt(snapshot, configuration_index, interface_index);
        if (!value) return invalid("interface index is out of range");
        *out_count = static_cast<std::uint32_t>(value->alternateSettings.size());
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_alternate_setting_at(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t interface_index, std::uint32_t alternate_index,
        usbhost_transport_alternate_setting_descriptor *out) {
    if (!validRecord(out)) return invalid("invalid alternate setting output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = alternateAt(
            snapshot, configuration_index, interface_index, alternate_index);
        if (!value) return invalid("alternate setting index is out of range");
        usbhost_transport_alternate_setting_descriptor result{};
        result.struct_size = sizeof(result); result.alternate_setting_index = alternate_index;
        result.snapshot_generation = value->generation.value();
        result.interface_number = value->interfaceNumber;
        result.alternate_setting = value->alternateSetting;
        result.interface_class = value->interfaceClass;
        result.interface_subclass = value->interfaceSubclass;
        result.interface_protocol = value->interfaceProtocol;
        result.endpoint_count = static_cast<std::uint32_t>(value->endpoints.size());
        result.additional_descriptor_count =
            static_cast<std::uint32_t>(value->additionalDescriptors.size());
        *out = result; return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_endpoint_count(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t interface_index, std::uint32_t alternate_index,
        std::uint32_t *out_count) {
    if (!out_count) return invalid("invalid endpoint count output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = alternateAt(
            snapshot, configuration_index, interface_index, alternate_index);
        if (!value) return invalid("alternate setting index is out of range");
        *out_count = static_cast<std::uint32_t>(value->endpoints.size());
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_endpoint_at(
        usbhost_transport_session handle, std::uint32_t configuration_index,
        std::uint32_t interface_index, std::uint32_t alternate_index,
        std::uint32_t endpoint_index, usbhost_transport_endpoint_descriptor *out) {
    if (!validRecord(out)) return invalid("invalid endpoint output");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        const auto *value = endpointAt(
            snapshot, configuration_index, interface_index, alternate_index, endpoint_index);
        if (!value) return invalid("endpoint index is out of range");
        usbhost_transport_endpoint_descriptor result{};
        result.struct_size = sizeof(result); result.endpoint_index = endpoint_index;
        result.snapshot_generation = value->generation.value();
        result.endpoint_address = value->address; result.endpoint_number = value->number;
        result.direction = static_cast<std::uint8_t>(value->direction);
        result.transfer_type = static_cast<std::uint8_t>(value->transferType);
        result.maximum_packet_size = value->maximumPacketSize; result.interval = value->interval;
        result.additional_descriptor_count =
            static_cast<std::uint32_t>(value->additionalDescriptors.size());
        *out = result; return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_get_additional_descriptor_at(
        usbhost_transport_session handle,
        const usbhost_transport_descriptor_location *location,
        std::uint8_t *destination, std::uint32_t capacity,
        std::uint8_t *out_type, std::uint32_t *out_actual_length) {
    if (!validRecord(location) || !out_type || !out_actual_length)
        return invalid("invalid additional descriptor arguments");
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        const auto snapshot = session->descriptorSnapshot();
        if (location->snapshot_generation != snapshot.generation.value())
            return finish(USBHOST_INVALID_STATE, "descriptor location generation is stale");
        const std::vector<usbhost::transport::AdditionalDescriptor> *descriptors = nullptr;
        if (location->scope == USBHOST_TRANSPORT_DESCRIPTOR_CONFIGURATION &&
            location->interface_index == 0 && location->alternate_setting_index == 0 &&
            location->endpoint_index == 0) {
            const auto *value = configurationAt(snapshot, location->configuration_index);
            if (value) descriptors = &value->additionalDescriptors;
        } else if (location->scope == USBHOST_TRANSPORT_DESCRIPTOR_ALTERNATE_SETTING &&
                   location->endpoint_index == 0) {
            const auto *value = alternateAt(snapshot, location->configuration_index,
                location->interface_index, location->alternate_setting_index);
            if (value) descriptors = &value->additionalDescriptors;
        } else if (location->scope == USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT) {
            const auto *value = endpointAt(snapshot, location->configuration_index,
                location->interface_index, location->alternate_setting_index,
                location->endpoint_index);
            if (value) descriptors = &value->additionalDescriptors;
        }
        if (!descriptors || location->additional_descriptor_index >= descriptors->size())
            return invalid("additional descriptor location is out of range");
        const auto &value = (*descriptors)[location->additional_descriptor_index];
        *out_type = value.type;
        *out_actual_length = static_cast<std::uint32_t>(value.bytes.size());
        if (capacity < value.bytes.size() || (!value.bytes.empty() && !destination))
            return invalid("additional descriptor destination is too small");
        if (!value.bytes.empty()) std::memcpy(destination, value.bytes.data(), value.bytes.size());
        return finish(USBHOST_OK);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_select_configuration(
        usbhost_transport_session handle, std::uint8_t value) {
    try {
        auto session = findSession(handle);
        return session ? finish(session->selectConfiguration(value))
                       : finish(USBHOST_INVALID_STATE, "unknown session");
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_claim_interface(
        usbhost_transport_session handle, std::uint8_t interface_number) {
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        InterfaceClaimToken token;
        const auto result = session->claimInterface(interface_number, token);
        if (result.status == USBHOST_OK) {
            std::lock_guard<std::mutex> lock(cApiStateMutex);
            claimTokens[handle][interface_number] = token;
        }
        return finish(result);
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_select_alternate_setting(
        usbhost_transport_session handle, std::uint8_t interface_number,
        std::uint8_t alternate_setting) {
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        std::lock_guard<std::mutex> lock(cApiStateMutex);
        auto sessionClaims = claimTokens.find(handle);
        if (sessionClaims == claimTokens.end() ||
            sessionClaims->second.find(interface_number) == sessionClaims->second.end())
            return finish(USBHOST_INVALID_STATE, "interface is not claimed");
        auto &token = sessionClaims->second[interface_number];
        return finish(session->selectAlternateSetting(token, alternate_setting));
    } catch (...) { return internalFailure(); }
}

usbhost_status usbhost_transport_release_interface(
        usbhost_transport_session handle, std::uint8_t interface_number) {
    try {
        auto session = findSession(handle);
        if (!session) return finish(USBHOST_INVALID_STATE, "unknown session");
        std::lock_guard<std::mutex> lock(cApiStateMutex);
        auto sessionClaims = claimTokens.find(handle);
        if (sessionClaims == claimTokens.end() ||
            sessionClaims->second.find(interface_number) == sessionClaims->second.end())
            return finish(USBHOST_INVALID_STATE, "interface is not claimed");
        const auto result = session->releaseInterface(sessionClaims->second[interface_number]);
        if (result.status == USBHOST_OK) {
            sessionClaims->second.erase(interface_number);
            if (sessionClaims->second.empty()) claimTokens.erase(sessionClaims);
        }
        return finish(result);
    } catch (...) { return internalFailure(); }
}

#if defined(USBHOST_BUILD_TESTS)
void usbhost_test_transport_install_fixture(int open_status) {
    fixtureOpenStatus = open_status;
    fixtureObservedFd = -1;
    fixtureFactory.state.reset();
}
int usbhost_test_transport_observed_fd(void) { return fixtureObservedFd; }
int usbhost_test_transport_backend_close_count(void) {
    return fixtureFactory.state ? fixtureFactory.state->closeCalls : 0;
}
#endif

}  // extern "C"

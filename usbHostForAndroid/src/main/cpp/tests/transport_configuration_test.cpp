#include <cstdint>
#include <memory>
#include <string>

#include "transport/session.hpp"

namespace {

int failures = 0;

#define CHECK_CONFIGURATION(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

usbhost::transport::ConfigurationDescriptor configuration(std::uint8_t value, bool active) {
    using namespace usbhost::transport;
    EndpointDescriptor endpoint;
    endpoint.address = static_cast<std::uint8_t>(0x80 | value);
    endpoint.number = value;
    endpoint.direction = Direction::In;
    endpoint.transferType = TransferType::Bulk;
    endpoint.interfaceNumber = 0;
    endpoint.alternateSetting = 0;
    endpoint.generation = SnapshotGeneration::initial();
    AlternateSettingDescriptor alternate;
    alternate.interfaceNumber = 0;
    alternate.alternateSetting = 0;
    alternate.generation = SnapshotGeneration::initial();
    alternate.endpoints.push_back(endpoint);
    InterfaceDescriptor interfaceDescriptor;
    interfaceDescriptor.interfaceNumber = 0;
    interfaceDescriptor.activeAlternateSetting = 0;
    interfaceDescriptor.generation = SnapshotGeneration::initial();
    interfaceDescriptor.alternateSettings.push_back(alternate);
    ConfigurationDescriptor result;
    result.configurationValue = value;
    result.active = active;
    result.generation = SnapshotGeneration::initial();
    result.interfaces.push_back(interfaceDescriptor);
    return result;
}

class ConfigurationBackend final : public usbhost::transport::UsbBackend {
public:
    ConfigurationBackend() {
        descriptor.generation = usbhost::transport::SnapshotGeneration::initial();
        descriptor.configurations = {configuration(1, true), configuration(2, false)};
    }
    const usbhost::transport::DeviceDescriptor &deviceDescriptor() const noexcept override {
        return descriptor;
    }
    usbhost::transport::BackendStatus selectConfiguration(std::uint8_t value) override {
        ++selectCalls;
        if (nextResult != usbhost::transport::BackendStatus::Success) {
            return nextResult;
        }
        for (auto &candidate : descriptor.configurations) {
            candidate.active = candidate.configurationValue == value;
        }
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus claimInterface(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus selectAlternateSetting(std::uint8_t, std::uint8_t) override {
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
    void close() noexcept override {}

    usbhost::transport::DeviceDescriptor descriptor;
    usbhost::transport::BackendStatus nextResult{usbhost::transport::BackendStatus::Success};
    int selectCalls{0};
};

class ConfigurationFactory final : public usbhost::transport::AuthorizedBackendFactory {
public:
    usbhost::transport::BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<usbhost::transport::UsbBackend> &outBackend,
            std::string &) override {
        auto backend = std::make_unique<ConfigurationBackend>();
        observed = backend.get();
        outBackend = std::move(backend);
        return usbhost::transport::BackendStatus::Success;
    }
    ConfigurationBackend *observed{nullptr};
};

void transactionalSelectionTest() {
    using namespace usbhost::transport;
    ConfigurationFactory factory;
    TransportError openError;
    auto session = TransportSession::open(5, factory, openError);
    CHECK_CONFIGURATION(session != nullptr);
    const DeviceDescriptor initial = session->descriptorSnapshot();
    const SnapshotGeneration staleEndpoint =
        initial.configurations[0].interfaces[0].alternateSettings[0].endpoints[0].generation;

    TransportError result = session->selectConfiguration(3);
    CHECK_CONFIGURATION(result.status == USBHOST_INVALID_ARGUMENT);
    CHECK_CONFIGURATION(factory.observed->selectCalls == 0);
    CHECK_CONFIGURATION(session->descriptorSnapshot().generation == initial.generation);

    factory.observed->nextResult = BackendStatus::Busy;
    result = session->selectConfiguration(2);
    CHECK_CONFIGURATION(result.status == USBHOST_BUSY);
    CHECK_CONFIGURATION(session->descriptorSnapshot().configurations[0].active);
    CHECK_CONFIGURATION(session->descriptorSnapshot().generation == initial.generation);

    factory.observed->nextResult = BackendStatus::Success;
    result = session->selectConfiguration(2);
    CHECK_CONFIGURATION(result.status == USBHOST_OK);
    const DeviceDescriptor refreshed = session->descriptorSnapshot();
    CHECK_CONFIGURATION(!refreshed.configurations[0].active);
    CHECK_CONFIGURATION(refreshed.configurations[1].active);
    CHECK_CONFIGURATION(refreshed.generation == initial.generation.next());
    const SnapshotGeneration currentEndpoint =
        refreshed.configurations[1].interfaces[0].alternateSettings[0].endpoints[0].generation;
    CHECK_CONFIGURATION(currentEndpoint == refreshed.generation);
    CHECK_CONFIGURATION(currentEndpoint != staleEndpoint);
}

}  // namespace

int runTransportConfigurationTest() {
    failures = 0;
    transactionalSelectionTest();
    return failures;
}

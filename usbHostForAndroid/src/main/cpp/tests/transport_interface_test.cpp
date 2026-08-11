#include <cstdint>
#include <memory>
#include <string>

#include "transport/session.hpp"

namespace {

int failures = 0;

#define CHECK_INTERFACE(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

usbhost::transport::DeviceDescriptor descriptorFixture() {
    using namespace usbhost::transport;
    DeviceDescriptor device;
    device.generation = SnapshotGeneration::initial();
    InterfaceDescriptor interfaceDescriptor;
    interfaceDescriptor.interfaceNumber = 3;
    interfaceDescriptor.activeAlternateSetting = 0;
    interfaceDescriptor.generation = device.generation;
    for (std::uint8_t alternateNumber = 0; alternateNumber < 2; ++alternateNumber) {
        AlternateSettingDescriptor alternate;
        alternate.interfaceNumber = 3;
        alternate.alternateSetting = alternateNumber;
        alternate.generation = device.generation;
        EndpointDescriptor endpoint;
        endpoint.address = static_cast<std::uint8_t>(0x81 + alternateNumber);
        endpoint.number = static_cast<std::uint8_t>(1 + alternateNumber);
        endpoint.direction = Direction::In;
        endpoint.transferType = TransferType::Bulk;
        endpoint.interfaceNumber = 3;
        endpoint.alternateSetting = alternateNumber;
        endpoint.generation = device.generation;
        alternate.endpoints.push_back(endpoint);
        interfaceDescriptor.alternateSettings.push_back(alternate);
    }
    ConfigurationDescriptor configuration;
    configuration.configurationValue = 1;
    configuration.active = true;
    configuration.generation = device.generation;
    configuration.interfaces.push_back(interfaceDescriptor);
    device.configurations.push_back(configuration);
    return device;
}

struct InterfaceBackendState {
    usbhost::transport::BackendStatus claimResult{usbhost::transport::BackendStatus::Success};
    usbhost::transport::BackendStatus alternateResult{usbhost::transport::BackendStatus::Success};
    usbhost::transport::BackendStatus releaseResult{usbhost::transport::BackendStatus::Success};
    int configurationCalls{0};
    int claimCalls{0};
    int alternateCalls{0};
    int releaseCalls{0};
    int closeCalls{0};
};

class InterfaceBackend final : public usbhost::transport::UsbBackend {
public:
    explicit InterfaceBackend(std::shared_ptr<InterfaceBackendState> state)
        : descriptor(descriptorFixture()), state_(std::move(state)) {}
    const usbhost::transport::DeviceDescriptor &deviceDescriptor() const noexcept override {
        return descriptor;
    }
    usbhost::transport::BackendStatus selectConfiguration(std::uint8_t) override {
        ++state_->configurationCalls;
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus claimInterface(std::uint8_t) override {
        ++state_->claimCalls;
        return state_->claimResult;
    }
    usbhost::transport::BackendStatus selectAlternateSetting(
            std::uint8_t, std::uint8_t alternateSetting) override {
        ++state_->alternateCalls;
        if (state_->alternateResult == usbhost::transport::BackendStatus::Success) {
            descriptor.configurations[0].interfaces[0].activeAlternateSetting = alternateSetting;
        }
        return state_->alternateResult;
    }
    usbhost::transport::BackendStatus releaseInterface(std::uint8_t) override {
        ++state_->releaseCalls;
        return state_->releaseResult;
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
    void close() noexcept override { ++state_->closeCalls; }

    usbhost::transport::DeviceDescriptor descriptor;

private:
    std::shared_ptr<InterfaceBackendState> state_;
};

class InterfaceFactory final : public usbhost::transport::AuthorizedBackendFactory {
public:
    usbhost::transport::BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<usbhost::transport::UsbBackend> &outBackend,
            std::string &) override {
        auto backend = std::make_unique<InterfaceBackend>(state);
        outBackend = std::move(backend);
        return usbhost::transport::BackendStatus::Success;
    }
    std::shared_ptr<InterfaceBackendState> state = std::make_shared<InterfaceBackendState>();
};

void claimAlternateReleaseTest() {
    using namespace usbhost::transport;
    InterfaceFactory factory;
    TransportError error;
    auto session = TransportSession::open(9, factory, error);
    CHECK_INTERFACE(session != nullptr);

    InterfaceClaimToken token;
    error = session->claimInterface(4, token);
    CHECK_INTERFACE(error.status == USBHOST_INVALID_ARGUMENT);
    CHECK_INTERFACE(factory.state->claimCalls == 0);

    factory.state->claimResult = BackendStatus::Busy;
    error = session->claimInterface(3, token);
    CHECK_INTERFACE(error.status == USBHOST_BUSY);
    CHECK_INTERFACE(!token.isValid());
    CHECK_INTERFACE(!session->descriptorSnapshot().configurations[0].interfaces[0].claimed);
    factory.state->claimResult = BackendStatus::Success;
    error = session->claimInterface(3, token);
    CHECK_INTERFACE(error.status == USBHOST_OK);
    CHECK_INTERFACE(token.isValid());
    CHECK_INTERFACE(session->descriptorSnapshot().configurations[0].interfaces[0].claimed);
    InterfaceClaimToken duplicate;
    error = session->claimInterface(3, duplicate);
    CHECK_INTERFACE(error.status == USBHOST_BUSY);
    CHECK_INTERFACE(factory.state->claimCalls == 2);

    error = session->selectConfiguration(1);
    CHECK_INTERFACE(error.status == USBHOST_BUSY);
    CHECK_INTERFACE(factory.state->configurationCalls == 0);

    const DeviceDescriptor beforeAlternate = session->descriptorSnapshot();
    const EndpointDescriptor staleEndpoint =
        beforeAlternate.configurations[0].interfaces[0].alternateSettings[0].endpoints[0];
    const InterfaceClaimToken staleToken = token;
    error = session->selectAlternateSetting(token, 7);
    CHECK_INTERFACE(error.status == USBHOST_INVALID_ARGUMENT);
    CHECK_INTERFACE(factory.state->alternateCalls == 0);
    factory.state->alternateResult = BackendStatus::Busy;
    error = session->selectAlternateSetting(token, 1);
    CHECK_INTERFACE(error.status == USBHOST_BUSY);
    CHECK_INTERFACE(token.snapshotGeneration == staleToken.snapshotGeneration);
    CHECK_INTERFACE(session->descriptorSnapshot().generation == beforeAlternate.generation);
    factory.state->alternateResult = BackendStatus::Success;
    error = session->selectAlternateSetting(token, 1);
    CHECK_INTERFACE(error.status == USBHOST_OK);
    CHECK_INTERFACE(token.snapshotGeneration != staleToken.snapshotGeneration);
    CHECK_INTERFACE(session->validateEndpoint(token, staleEndpoint).status == USBHOST_INVALID_STATE);
    const DeviceDescriptor afterAlternate = session->descriptorSnapshot();
    const EndpointDescriptor currentEndpoint =
        afterAlternate.configurations[0].interfaces[0].alternateSettings[1].endpoints[0];
    CHECK_INTERFACE(session->validateEndpoint(token, currentEndpoint).status == USBHOST_OK);
    CHECK_INTERFACE(session->releaseInterface(staleToken).status == USBHOST_INVALID_STATE);
    CHECK_INTERFACE(session->releaseInterface(token).status == USBHOST_OK);
    CHECK_INTERFACE(session->releaseInterface(token).status == USBHOST_INVALID_STATE);

    InterfaceClaimToken replacement;
    CHECK_INTERFACE(session->claimInterface(3, replacement).status == USBHOST_OK);
    CHECK_INTERFACE(replacement.claimGeneration != token.claimGeneration);
    factory.state->releaseResult = BackendStatus::Busy;
    CHECK_INTERFACE(session->releaseInterface(replacement).status == USBHOST_BUSY);
    CHECK_INTERFACE(session->descriptorSnapshot().configurations[0].interfaces[0].claimed);
    factory.state->releaseResult = BackendStatus::Success;
    session->close();
    CHECK_INTERFACE(factory.state->releaseCalls == 3);
    CHECK_INTERFACE(factory.state->closeCalls == 1);
}

}  // namespace

int runTransportInterfaceTest() {
    failures = 0;
    claimAlternateReleaseTest();
    return failures;
}

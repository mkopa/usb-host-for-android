#include <array>
#include <memory>

#include "tests/scripted_usb_backend.hpp"
#include "transport/session.hpp"

namespace {
using namespace usbhost::transport;
int failures;
#define CHECK_ENDPOINT(value) do { if (!(value)) ++failures; } while (0)

EndpointDescriptor endpoint(std::uint8_t address, TransferType type) {
    EndpointDescriptor value;
    value.address = address;
    value.number = address & 0x0f;
    value.direction = Direction::In;
    value.transferType = type;
    value.interfaceNumber = 3;
    value.alternateSetting = 0;
    value.generation = SnapshotGeneration::initial();
    return value;
}

class Factory final : public AuthorizedBackendFactory {
public:
    Factory() : backend(std::make_unique<ScriptedUsbBackend>()) {
        auto &device = backend->mutableDeviceDescriptor();
        device.generation = SnapshotGeneration::initial();
        AlternateSettingDescriptor alternate;
        alternate.interfaceNumber = 3;
        alternate.generation = device.generation;
        alternate.endpoints = {endpoint(0x81, TransferType::Bulk),
                               endpoint(0x82, TransferType::Interrupt)};
        InterfaceDescriptor iface;
        iface.interfaceNumber = 3;
        iface.generation = device.generation;
        iface.alternateSettings.push_back(alternate);
        ConfigurationDescriptor configuration;
        configuration.configurationValue = 1;
        configuration.active = true;
        configuration.generation = device.generation;
        configuration.interfaces.push_back(iface);
        device.configurations.push_back(configuration);
    }
    BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<UsbBackend> &out, std::string &) override {
        out = std::move(backend); return BackendStatus::Success;
    }
    std::unique_ptr<ScriptedUsbBackend> backend;
};

EndpointTransferRequest request(const EndpointDescriptor &endpoint, std::uint32_t length) {
    EndpointTransferRequest value;
    value.transferType = endpoint.transferType;
    value.endpointAddress = endpoint.address;
    value.direction = endpoint.direction;
    value.buffer = {0, length};
    value.timeoutMilliseconds = 1000;
    value.generation = endpoint.generation;
    return value;
}
}

int runTransportEndpointTransferTest() {
    failures = 0;
    Factory factory;
    ScriptedUsbBackend *backend = factory.backend.get();
    backend->enqueue({TransferType::Bulk, BackendStatus::Success, {1, 2, 3}, 3, false, {}});
    backend->enqueue({TransferType::Interrupt, BackendStatus::Success, {4}, 1, false, {}});
    TransportError error;
    auto session = TransportSession::open(71, factory, error);
    InterfaceClaimToken claim;
    CHECK_ENDPOINT(session->claimInterface(3, claim).status == USBHOST_OK);
    const auto descriptor = session->descriptorSnapshot();
    const auto &endpoints = descriptor.configurations[0].interfaces[0]
        .alternateSettings[0].endpoints;
    std::array<std::uint8_t, 8> bytes{};
    TransferResult bulk = session->endpointTransfer(
        claim, endpoints[0], request(endpoints[0], bytes.size()), {bytes.data(), bytes.size()});
    CHECK_ENDPOINT(bulk.status == USBHOST_OK && bulk.actualLength == 3 && bytes[2] == 3);
    TransferResult interrupt = session->endpointTransfer(
        claim, endpoints[1], request(endpoints[1], bytes.size()), {bytes.data(), bytes.size()});
    CHECK_ENDPOINT(interrupt.status == USBHOST_OK && interrupt.actualLength == 1 && bytes[0] == 4);
    CHECK_ENDPOINT(session->close() == USBHOST_OK);
    return failures;
}

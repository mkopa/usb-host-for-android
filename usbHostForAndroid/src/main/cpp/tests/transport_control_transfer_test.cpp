#include <array>
#include <memory>

#include "tests/scripted_usb_backend.hpp"
#include "transport/session.hpp"

namespace {
using namespace usbhost::transport;
int failures;
#define CHECK_CONTROL(value) do { if (!(value)) ++failures; } while (0)

class Factory final : public AuthorizedBackendFactory {
public:
    Factory() : backend(std::make_unique<ScriptedUsbBackend>()) {
        backend->mutableDeviceDescriptor().generation = SnapshotGeneration::initial();
    }
    BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<UsbBackend> &out, std::string &) override {
        out = std::move(backend);
        return BackendStatus::Success;
    }
    std::unique_ptr<ScriptedUsbBackend> backend;
};

ControlRequest request(Direction direction, std::uint32_t offset, std::uint32_t length) {
    ControlRequest value;
    value.requestType = direction == Direction::In ? 0x80 : 0x00;
    value.direction = direction;
    value.buffer = {offset, length};
    value.timeoutMilliseconds = 1000;
    value.generation = SnapshotGeneration::initial();
    return value;
}
}

int runTransportControlTransferTest() {
    failures = 0;
    Factory factory;
    ScriptedUsbBackend *backend = factory.backend.get();
    backend->enqueue({TransferType::Control, BackendStatus::Success, {7, 8}, 2, false, {}});
    backend->enqueue({TransferType::Control, BackendStatus::Success, {}, 3, false, {}});
    backend->enqueue({TransferType::Control, BackendStatus::Success, {}, 0, false, {}});
    TransportError openError;
    auto session = TransportSession::open(70, factory, openError);
    CHECK_CONTROL(session && openError.status == USBHOST_OK);

    std::array<std::uint8_t, 6> bytes{0xaa, 0, 0, 0, 0xbb, 0xcc};
    TransferResult input = session->controlTransfer(
        request(Direction::In, 1, 3), {bytes.data(), bytes.size()});
    CHECK_CONTROL(input.status == USBHOST_OK && input.actualLength == 2);
    CHECK_CONTROL(bytes[0] == 0xaa && bytes[1] == 7 && bytes[2] == 8 && bytes[3] == 0);

    TransferResult output = session->controlTransfer(
        request(Direction::Out, 1, 3), {bytes.data(), bytes.size()});
    CHECK_CONTROL(output.status == USBHOST_OK && output.actualLength == 3);

    TransferResult zero = session->controlTransfer(
        request(Direction::Out, 0, 0), {nullptr, 0});
    CHECK_CONTROL(zero.status == USBHOST_OK && zero.actualLength == 0);
    CHECK_CONTROL(session->close() == USBHOST_OK);
    return failures;
}

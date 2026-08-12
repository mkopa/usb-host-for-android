#include <array>
#include <memory>

#include "tests/scripted_usb_backend.hpp"
#include "transport/session.hpp"

namespace {
using namespace usbhost::transport;
int failures;
#define CHECK_TERMINAL(value) do { if (!(value)) ++failures; } while (0)
class Factory final : public AuthorizedBackendFactory {
public:
    Factory() : backend(std::make_unique<ScriptedUsbBackend>()) {
        backend->mutableDeviceDescriptor().generation = SnapshotGeneration::initial();
    }
    BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<UsbBackend> &out, std::string &) override {
        out = std::move(backend); return BackendStatus::Success;
    }
    std::unique_ptr<ScriptedUsbBackend> backend;
};
ControlRequest request() {
    ControlRequest value;
    value.requestType = 0x80;
    value.direction = Direction::In;
    value.buffer = {0, 4};
    value.timeoutMilliseconds = 20;
    value.generation = SnapshotGeneration::initial();
    return value;
}
}

int runTransportTerminalResultTest() {
    failures = 0;
    Factory factory;
    ScriptedUsbBackend *backend = factory.backend.get();
    backend->enqueue({TransferType::Control, BackendStatus::Timeout, {1, 2}, 2, false, "timeout"});
    backend->enqueue({TransferType::Control, BackendStatus::Stall, {3}, 1, false, "stall"});
    backend->enqueue({TransferType::Control, BackendStatus::Disconnected, {}, 0, false,
                      "device disconnected"});
    TransportError error;
    auto session = TransportSession::open(72, factory, error);
    std::array<std::uint8_t, 4> bytes{};
    const TransferResult timeout = session->controlTransfer(request(), {bytes.data(), bytes.size()});
    CHECK_TERMINAL(timeout.status == USBHOST_TIMEOUT && timeout.actualLength == 2);
    CHECK_TERMINAL(bytes[0] == 1 && bytes[1] == 2 && session->state() == SessionState::Open);
    const TransferResult stall = session->controlTransfer(request(), {bytes.data(), bytes.size()});
    CHECK_TERMINAL(stall.status == USBHOST_STALL && stall.actualLength == 1);
    CHECK_TERMINAL(session->state() == SessionState::Open);
    const TransferResult disconnected = session->controlTransfer(
        request(), {bytes.data(), bytes.size()});
    CHECK_TERMINAL(disconnected.status == USBHOST_DISCONNECTED);
    CHECK_TERMINAL(session->state() == SessionState::Failed);
    CHECK_TERMINAL(session->controlTransfer(request(), {bytes.data(), bytes.size()}).status
                   == USBHOST_INVALID_STATE);
    CHECK_TERMINAL(session->close() == USBHOST_OK);
    return failures;
}

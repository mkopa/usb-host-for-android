#include <memory>
#include <string>

#include "android/android_usb_backend.hpp"

namespace {
using namespace usbhost::android;
using namespace usbhost::transport;
int failures;
#define CHECK_ANDROID_TRANSFER(value) do { if (!(value)) ++failures; } while (0)

struct State {
    CompletionCallback pending;
    TransferType observedType{TransferType::Isochronous};
    OperationId cancelled{kInvalidOperationId};
};
int duplicate(void *, int fd) { return fd + 1; }
void closeFd(void *, int) {}
BackendStatus runtime(void *data, AndroidUsbRuntimeLease &lease, std::string &) {
    lease.context = data;
    lease.owner = std::shared_ptr<void>(data, [](void *) {});
    return BackendStatus::Success;
}
BackendStatus wrap(void *data, LibusbContext, int, void **handle, std::string &) {
    *handle = data; return BackendStatus::Success;
}
BackendStatus descriptors(void *, void *, RawDescriptorSet &out, std::string &) {
    out.deviceDescriptor = {18, 1, 0, 2, 0, 0, 0, 64, 0x34, 0x12, 0x78, 0x56,
                            0, 1, 0, 0, 0, 1};
    out.configurationDescriptors = {{9, 2, 9, 0, 0, 1, 0, 0x80, 50}};
    out.activeConfigurationValue = 1;
    out.generation = SnapshotGeneration::initial();
    return BackendStatus::Success;
}
void closeHandle(void *, void *) {}
BackendStatus byteOperation(void *, void *, std::uint8_t) { return BackendStatus::Success; }
BackendStatus alternateOperation(void *, void *, std::uint8_t, std::uint8_t) {
    return BackendStatus::Success;
}
OperationId submitControl(void *data, void *, const ControlRequest &, MutableBufferView,
                          CompletionCallback completion) {
    auto &state = *static_cast<State *>(data);
    state.observedType = TransferType::Control;
    state.pending = std::move(completion);
    return 41;
}
OperationId submitEndpoint(void *data, void *, const EndpointTransferRequest &request,
                           MutableBufferView, CompletionCallback completion) {
    auto &state = *static_cast<State *>(data);
    state.observedType = request.transferType;
    state.pending = std::move(completion);
    return 42;
}
bool cancelTransfer(void *data, OperationId operation) {
    auto &state = *static_cast<State *>(data);
    state.cancelled = operation;
    return true;
}
AndroidUsbBackendHooks hooks(State &state) {
    return {&state, duplicate, closeFd, runtime, wrap, descriptors, closeHandle,
            byteOperation, byteOperation, alternateOperation, byteOperation,
            submitControl, submitEndpoint, cancelTransfer};
}
}

int runAndroidUsbTransferContractTest() {
    failures = 0;
    State state;
    AndroidUsbBackendFactory factory(hooks(state));
    std::unique_ptr<UsbBackend> backend;
    std::string diagnostic;
    CHECK_ANDROID_TRANSFER(factory.openAuthorizedFileDescriptor(10, backend, diagnostic)
                           == BackendStatus::Success);
    ControlRequest control;
    control.direction = Direction::In;
    control.timeoutMilliseconds = 10;
    int callbackCount = 0;
    BackendCompletion observed;
    CHECK_ANDROID_TRANSFER(backend->submitControl(control, {nullptr, 0},
        [&](BackendCompletion result) { ++callbackCount; observed = std::move(result); }) == 41);
    CHECK_ANDROID_TRANSFER(state.observedType == TransferType::Control && callbackCount == 0);
    state.pending({BackendStatus::Timeout, 2, "timeout"});
    CHECK_ANDROID_TRANSFER(callbackCount == 1 && observed.status == BackendStatus::Timeout
                           && observed.actualLength == 2);
    EndpointTransferRequest endpoint;
    endpoint.transferType = TransferType::Interrupt;
    CHECK_ANDROID_TRANSFER(backend->submitEndpoint(endpoint, {nullptr, 0},
        [&](BackendCompletion result) { ++callbackCount; observed = std::move(result); }) == 42);
    CHECK_ANDROID_TRANSFER(state.observedType == TransferType::Interrupt);
    CHECK_ANDROID_TRANSFER(backend->cancel(42) && state.cancelled == 42);
    state.pending({BackendStatus::Cancelled, 1, "cancelled"});
    CHECK_ANDROID_TRANSFER(callbackCount == 2 && observed.status == BackendStatus::Cancelled
                           && observed.actualLength == 1);
    backend->close();
    return failures;
}

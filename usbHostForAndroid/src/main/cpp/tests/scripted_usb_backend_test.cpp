#include <array>
#include <cstdint>
#include <string>

#include "tests/scripted_usb_backend.hpp"

namespace {

int failures = 0;

#define CHECK_BACKEND(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

usbhost::transport::EndpointTransferRequest bulkInRequest() {
    using namespace usbhost::transport;
    EndpointTransferRequest request;
    request.transferType = TransferType::Bulk;
    request.endpointAddress = 0x81;
    request.direction = Direction::In;
    request.buffer = {0, 8};
    request.timeoutMilliseconds = 1000;
    request.generation = SnapshotGeneration::initial();
    return request;
}

void immediateAndDeferredTest() {
    using namespace usbhost::transport;
    ScriptedUsbBackend backend;
    backend.enqueue({TransferType::Bulk, BackendStatus::Success, {1, 2, 3}, 3, false,
                     "short packet"});
    std::array<std::uint8_t, 8> bytes{};
    int callbacks = 0;
    BackendCompletion completion;
    const OperationId immediate = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++callbacks;
            completion = std::move(result);
        });
    CHECK_BACKEND(immediate != kInvalidOperationId);
    CHECK_BACKEND(callbacks == 1);
    CHECK_BACKEND(completion.status == BackendStatus::Success);
    CHECK_BACKEND(completion.actualLength == 3);
    CHECK_BACKEND(bytes[0] == 1 && bytes[2] == 3 && bytes[3] == 0);

    backend.enqueue({TransferType::Bulk, BackendStatus::Timeout, {}, 0, true, "timeout"});
    callbacks = 0;
    const OperationId deferred = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++callbacks;
            completion = std::move(result);
        });
    CHECK_BACKEND(deferred != kInvalidOperationId);
    CHECK_BACKEND(callbacks == 0);
    CHECK_BACKEND(backend.pendingCount() == 1);
    CHECK_BACKEND(backend.completeNext());
    CHECK_BACKEND(callbacks == 1);
    CHECK_BACKEND(completion.status == BackendStatus::Timeout);
    CHECK_BACKEND(!backend.completeNext());
}

void cancellationAndBusyTest() {
    using namespace usbhost::transport;
    ScriptedUsbBackend backend;
    backend.enqueue({TransferType::Bulk, BackendStatus::Success, {9, 8}, 2, true, "pending"});
    std::array<std::uint8_t, 8> bytes{};
    int firstCallbacks = 0;
    BackendCompletion first;
    const OperationId pending = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++firstCallbacks;
            first = std::move(result);
        });

    int busyCallbacks = 0;
    BackendCompletion busy;
    const OperationId rejected = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++busyCallbacks;
            busy = std::move(result);
        });
    CHECK_BACKEND(rejected == kInvalidOperationId);
    CHECK_BACKEND(busyCallbacks == 1);
    CHECK_BACKEND(busy.status == BackendStatus::Busy);

    CHECK_BACKEND(backend.cancel(pending));
    CHECK_BACKEND(firstCallbacks == 1);
    CHECK_BACKEND(first.status == BackendStatus::Cancelled);
    CHECK_BACKEND(first.actualLength == 2);
    CHECK_BACKEND(bytes[0] == 9 && bytes[1] == 8);
    CHECK_BACKEND(!backend.cancel(pending));
}

void missingScriptAndCloseTest() {
    using namespace usbhost::transport;
    ScriptedUsbBackend backend;
    std::array<std::uint8_t, 8> bytes{};
    int callbacks = 0;
    BackendCompletion completion;
    const OperationId missing = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++callbacks;
            completion = std::move(result);
        });
    CHECK_BACKEND(missing == kInvalidOperationId);
    CHECK_BACKEND(callbacks == 1);
    CHECK_BACKEND(completion.status == BackendStatus::InternalFailure);

    backend.enqueue({TransferType::Bulk, BackendStatus::Success, {}, 0, true, "pending"});
    const OperationId pending = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++callbacks;
            completion = std::move(result);
        });
    CHECK_BACKEND(pending != kInvalidOperationId);
    backend.close();
    backend.close();
    CHECK_BACKEND(backend.isClosed());
    CHECK_BACKEND(completion.status == BackendStatus::Cancelled);
    CHECK_BACKEND(callbacks == 2);

    const OperationId closed = backend.submitEndpoint(
        bulkInRequest(), {bytes.data(), static_cast<std::uint32_t>(bytes.size())},
        [&](BackendCompletion result) {
            ++callbacks;
            completion = std::move(result);
        });
    CHECK_BACKEND(closed == kInvalidOperationId);
    CHECK_BACKEND(completion.status == BackendStatus::InternalFailure);
    CHECK_BACKEND(callbacks == 3);
}

}  // namespace

int runScriptedUsbBackendTest() {
    failures = 0;
    immediateAndDeferredTest();
    cancellationAndBusyTest();
    missingScriptAndCloseTest();
    return failures;
}

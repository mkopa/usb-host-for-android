#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "transport/session.hpp"

namespace {
using namespace usbhost::transport;
int failures;
#define CHECK_CANCEL(value) do { if (!(value)) ++failures; } while (0)

class PendingBackend final : public UsbBackend {
public:
    PendingBackend(bool cancelSucceeds, bool closeCompletes,
                   std::shared_ptr<int> closeCount)
        : cancelSucceeds_(cancelSucceeds), closeCompletes_(closeCompletes),
          closeCount_(std::move(closeCount)) {
        descriptor_.generation = SnapshotGeneration::initial();
    }
    const DeviceDescriptor &deviceDescriptor() const noexcept override { return descriptor_; }
    BackendStatus selectConfiguration(std::uint8_t) override { return BackendStatus::Success; }
    BackendStatus claimInterface(std::uint8_t) override { return BackendStatus::Success; }
    BackendStatus selectAlternateSetting(std::uint8_t, std::uint8_t) override {
        return BackendStatus::Success;
    }
    BackendStatus releaseInterface(std::uint8_t) override { return BackendStatus::Success; }
    OperationId submitControl(const ControlRequest &, MutableBufferView buffer,
                              CompletionCallback completion) override {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_ = buffer;
        completion_ = std::move(completion);
        submitted_ = true;
        submittedCondition_.notify_all();
        return 1;
    }
    OperationId submitEndpoint(const EndpointTransferRequest &, MutableBufferView,
                               CompletionCallback completion) override {
        completion({BackendStatus::UnsupportedOperation, 0, {}});
        return kInvalidOperationId;
    }
    bool cancel(OperationId operation) override {
        CompletionCallback completion;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!cancelSucceeds_ || operation != 1 || !completion_) return false;
            if (buffer_.capacity >= 2) { buffer_.data[0] = 0x31; buffer_.data[1] = 0x32; }
            completion = std::move(completion_);
        }
        completion({BackendStatus::Cancelled, 2, "cancelled"});
        return true;
    }
    void close() noexcept override {
        CompletionCallback completion;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++*closeCount_;
            completion = std::move(completion_);
        }
        if (completion && closeCompletes_) {
            completion({BackendStatus::Cancelled, 0, "closed"});
        }
    }
    void waitUntilSubmitted() {
        std::unique_lock<std::mutex> lock(mutex_);
        submittedCondition_.wait(lock, [this] { return submitted_; });
    }
private:
    DeviceDescriptor descriptor_;
    bool cancelSucceeds_;
    bool closeCompletes_;
    std::mutex mutex_;
    std::condition_variable submittedCondition_;
    bool submitted_{false};
    MutableBufferView buffer_;
    CompletionCallback completion_;
    std::shared_ptr<int> closeCount_;
};

class Factory final : public AuthorizedBackendFactory {
public:
    explicit Factory(bool cancelSucceeds, bool closeCompletes = true)
        : closeCount(std::make_shared<int>(0)),
          backend(std::make_unique<PendingBackend>(
                  cancelSucceeds, closeCompletes, closeCount)), raw(backend.get()) {}
    BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<UsbBackend> &out, std::string &) override {
        out = std::move(backend); return BackendStatus::Success;
    }
    std::shared_ptr<int> closeCount;
    std::unique_ptr<PendingBackend> backend;
    PendingBackend *raw;
};

ControlRequest request() {
    ControlRequest value;
    value.requestType = 0x80;
    value.direction = Direction::In;
    value.buffer = {0, 8};
    value.timeoutMilliseconds = 1000;
    value.generation = SnapshotGeneration::initial();
    return value;
}

void explicitCancellation() {
    Factory factory(true);
    TransportError error;
    auto session = TransportSession::open(80, factory, error);
    std::array<std::uint8_t, 8> bytes{};
    TransferResult result;
    std::thread worker([&] {
        result = session->controlTransfer(request(), {bytes.data(), bytes.size()});
    });
    factory.raw->waitUntilSubmitted();
    CHECK_CANCEL(session->cancelActiveTransfer().status == USBHOST_OK);
    worker.join();
    CHECK_CANCEL(result.status == USBHOST_CANCELLED && result.actualLength == 2);
    CHECK_CANCEL(bytes[0] == 0x31 && bytes[1] == 0x32);
    CHECK_CANCEL(session->state() == SessionState::Open);
    session->close();
}

void closeCancelsWithinDeadline() {
    Factory factory(false);
    TransportError error;
    auto session = TransportSession::open(81, factory, error);
    std::array<std::uint8_t, 8> bytes{};
    TransferResult result;
    std::thread worker([&] {
        result = session->controlTransfer(request(), {bytes.data(), bytes.size()});
    });
    factory.raw->waitUntilSubmitted();
    const auto started = std::chrono::steady_clock::now();
    CHECK_CANCEL(session->close() == USBHOST_OK);
    const auto elapsed = std::chrono::steady_clock::now() - started;
    worker.join();
    CHECK_CANCEL(elapsed < std::chrono::seconds(2));
    CHECK_CANCEL(result.status == USBHOST_CANCELLED);
    CHECK_CANCEL(session->state() == SessionState::Closed);
    CHECK_CANCEL(*factory.closeCount == 1);
}

void missingCallbackStillMeetsDeadline() {
    Factory factory(false, false);
    TransportError error;
    auto session = TransportSession::open(82, factory, error);
    std::array<std::uint8_t, 8> bytes{};
    TransferResult result;
    std::thread worker([&] {
        result = session->controlTransfer(request(), {bytes.data(), bytes.size()});
    });
    factory.raw->waitUntilSubmitted();
    const auto started = std::chrono::steady_clock::now();
    CHECK_CANCEL(session->close() == USBHOST_OK);
    const auto elapsed = std::chrono::steady_clock::now() - started;
    worker.join();
    CHECK_CANCEL(elapsed < std::chrono::seconds(2));
    CHECK_CANCEL(result.status == USBHOST_CANCELLED);
}
}

int runTransportCancellationTest() {
    failures = 0;
    explicitCancellation();
    closeCancelsWithinDeadline();
    missingCallbackStillMeetsDeadline();
    return failures;
}

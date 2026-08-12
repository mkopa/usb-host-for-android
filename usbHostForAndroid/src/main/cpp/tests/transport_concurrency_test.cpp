#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "transport/session.hpp"

namespace {
using namespace usbhost::transport;
int failures;
#define CHECK_CONCURRENCY(value) do { if (!(value)) ++failures; } while (0)

class GateBackend final : public UsbBackend {
public:
    GateBackend() {
        descriptor_.generation = SnapshotGeneration::initial();
        ConfigurationDescriptor configuration;
        configuration.configurationValue = 1;
        configuration.active = true;
        configuration.generation = descriptor_.generation;
        descriptor_.configurations.push_back(configuration);
    }
    const DeviceDescriptor &deviceDescriptor() const noexcept override { return descriptor_; }
    BackendStatus selectConfiguration(std::uint8_t) override {
        ++configurationCalls; return BackendStatus::Success;
    }
    BackendStatus claimInterface(std::uint8_t) override { return BackendStatus::Success; }
    BackendStatus selectAlternateSetting(std::uint8_t, std::uint8_t) override {
        return BackendStatus::Success;
    }
    BackendStatus releaseInterface(std::uint8_t) override { return BackendStatus::Success; }
    OperationId submitControl(const ControlRequest &, MutableBufferView,
                              CompletionCallback completion) override {
        std::lock_guard<std::mutex> lock(mutex_);
        completion_ = std::move(completion);
        submitted_ = true;
        condition_.notify_all();
        return 1;
    }
    OperationId submitEndpoint(const EndpointTransferRequest &, MutableBufferView,
                               CompletionCallback completion) override {
        completion({BackendStatus::UnsupportedOperation, 0, {}});
        return kInvalidOperationId;
    }
    bool cancel(OperationId) override { return false; }
    void close() noexcept override { complete(BackendStatus::Cancelled); }
    void waitSubmitted() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return submitted_; });
    }
    void complete(BackendStatus status = BackendStatus::Success) noexcept {
        CompletionCallback completion;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            completion = std::move(completion_);
        }
        if (completion) completion({status, 0, {}});
    }
    int configurationCalls{0};
private:
    DeviceDescriptor descriptor_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool submitted_{false};
    CompletionCallback completion_;
};

class Factory final : public AuthorizedBackendFactory {
public:
    Factory() : backend(std::make_unique<GateBackend>()), raw(backend.get()) {}
    BackendStatus openAuthorizedFileDescriptor(
            int, std::unique_ptr<UsbBackend> &out, std::string &) override {
        out = std::move(backend); return BackendStatus::Success;
    }
    std::unique_ptr<GateBackend> backend;
    GateBackend *raw;
};

ControlRequest request() {
    ControlRequest value;
    value.requestType = 0;
    value.direction = Direction::Out;
    value.buffer = {0, 0};
    value.timeoutMilliseconds = 1000;
    value.generation = SnapshotGeneration::initial();
    return value;
}

std::shared_ptr<TransportSession> open(Factory &factory) {
    TransportError error;
    return TransportSession::open(90, factory, error);
}

void sameSessionExclusion() {
    Factory factory;
    auto session = open(factory);
    TransferResult first;
    std::thread worker([&] { first = session->controlTransfer(request(), {nullptr, 0}); });
    factory.raw->waitSubmitted();
    CHECK_CONCURRENCY(session->controlTransfer(request(), {nullptr, 0}).status == USBHOST_BUSY);
    CHECK_CONCURRENCY(session->selectConfiguration(1).status == USBHOST_BUSY);
    CHECK_CONCURRENCY(factory.raw->configurationCalls == 0);
    factory.raw->complete();
    worker.join();
    CHECK_CONCURRENCY(first.status == USBHOST_OK);
    session->close();
}

void separateSessionsRunTogether() {
    Factory firstFactory;
    Factory secondFactory;
    auto first = open(firstFactory);
    auto second = open(secondFactory);
    TransferResult firstResult;
    TransferResult secondResult;
    std::thread firstWorker([&] { firstResult = first->controlTransfer(request(), {nullptr, 0}); });
    std::thread secondWorker([&] { secondResult = second->controlTransfer(request(), {nullptr, 0}); });
    firstFactory.raw->waitSubmitted();
    secondFactory.raw->waitSubmitted();
    firstFactory.raw->complete();
    secondFactory.raw->complete();
    firstWorker.join();
    secondWorker.join();
    CHECK_CONCURRENCY(firstResult.status == USBHOST_OK && secondResult.status == USBHOST_OK);
    first->close();
    second->close();
}
}

int runTransportConcurrencyTest() {
    failures = 0;
    sameSessionExclusion();
    separateSessionsRunTogether();
    return failures;
}

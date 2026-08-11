#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "transport/session.hpp"

namespace {

int failures = 0;

#define CHECK_LIFECYCLE(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

struct BackendCounters {
    int closeCalls{0};
};

class CountingBackend final : public usbhost::transport::UsbBackend {
public:
    explicit CountingBackend(std::shared_ptr<BackendCounters> counters)
        : counters_(std::move(counters)) {
        descriptor_.vendorId = 0x1234;
        descriptor_.generation = usbhost::transport::SnapshotGeneration::initial();
    }

    const usbhost::transport::DeviceDescriptor &deviceDescriptor() const noexcept override {
        return descriptor_;
    }
    usbhost::transport::BackendStatus selectConfiguration(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus claimInterface(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus selectAlternateSetting(
            std::uint8_t, std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::BackendStatus releaseInterface(std::uint8_t) override {
        return usbhost::transport::BackendStatus::Success;
    }
    usbhost::transport::OperationId submitControl(
            const usbhost::transport::ControlRequest &,
            usbhost::transport::MutableBufferView,
            usbhost::transport::CompletionCallback) override {
        return usbhost::transport::kInvalidOperationId;
    }
    usbhost::transport::OperationId submitEndpoint(
            const usbhost::transport::EndpointTransferRequest &,
            usbhost::transport::MutableBufferView,
            usbhost::transport::CompletionCallback) override {
        return usbhost::transport::kInvalidOperationId;
    }
    bool cancel(usbhost::transport::OperationId) override {
        return false;
    }
    void close() noexcept override {
        if (!closed_) {
            closed_ = true;
            ++counters_->closeCalls;
        }
    }

private:
    std::shared_ptr<BackendCounters> counters_;
    usbhost::transport::DeviceDescriptor descriptor_;
    bool closed_{false};
};

class FakeFactory final : public usbhost::transport::AuthorizedBackendFactory {
public:
    usbhost::transport::BackendStatus openAuthorizedFileDescriptor(
            int borrowedFileDescriptor,
            std::unique_ptr<usbhost::transport::UsbBackend> &outBackend,
            std::string &outDiagnostic) override {
        ++openCalls;
        observedBorrowedFileDescriptor = borrowedFileDescriptor;
        if (result != usbhost::transport::BackendStatus::Success || returnPartialBackend) {
            outBackend = std::make_unique<CountingBackend>(counters);
        }
        if (result != usbhost::transport::BackendStatus::Success) {
            outDiagnostic = "open failed";
            return result;
        }
        if (!outBackend) {
            outBackend = std::make_unique<CountingBackend>(counters);
        }
        return result;
    }

    std::shared_ptr<BackendCounters> counters = std::make_shared<BackendCounters>();
    usbhost::transport::BackendStatus result{usbhost::transport::BackendStatus::Success};
    int openCalls{0};
    int observedBorrowedFileDescriptor{-1};
    bool returnPartialBackend{false};
};

void successfulOpenAndIdempotentCloseTest() {
    using namespace usbhost::transport;
    FakeFactory factory;
    TransportError error;
    auto session = TransportSession::open(51, factory, error);
    CHECK_LIFECYCLE(session != nullptr);
    CHECK_LIFECYCLE(error.status == USBHOST_OK);
    CHECK_LIFECYCLE(factory.openCalls == 1);
    CHECK_LIFECYCLE(factory.observedBorrowedFileDescriptor == 51);
    CHECK_LIFECYCLE(session->state() == SessionState::Open);
    CHECK_LIFECYCLE(session->descriptorSnapshot().vendorId == 0x1234);

    CHECK_LIFECYCLE(session->close() == USBHOST_OK);
    CHECK_LIFECYCLE(session->close() == USBHOST_OK);
    CHECK_LIFECYCLE(session->state() == SessionState::Closed);
    CHECK_LIFECYCLE(factory.counters->closeCalls == 1);
}

void failedOpenCleansPartialBackendTest() {
    using namespace usbhost::transport;
    FakeFactory factory;
    factory.result = BackendStatus::PermissionDenied;
    factory.returnPartialBackend = true;
    TransportError error;
    auto session = TransportSession::open(52, factory, error);
    CHECK_LIFECYCLE(session == nullptr);
    CHECK_LIFECYCLE(error.status == USBHOST_PERMISSION_DENIED);
    CHECK_LIFECYCLE(factory.counters->closeCalls == 1);

    FakeFactory invalidFactory;
    session = TransportSession::open(-1, invalidFactory, error);
    CHECK_LIFECYCLE(session == nullptr);
    CHECK_LIFECYCLE(error.status == USBHOST_INVALID_ARGUMENT);
    CHECK_LIFECYCLE(invalidFactory.openCalls == 0);
}

void callerDescriptorRemainsBorrowedTest() {
    using namespace usbhost::transport;
    FakeFactory factory;
    TransportError error;
    constexpr int callerOwnedDescriptor = 73;
    auto session = TransportSession::open(callerOwnedDescriptor, factory, error);
    CHECK_LIFECYCLE(session != nullptr);
    session.reset();
    CHECK_LIFECYCLE(factory.observedBorrowedFileDescriptor == callerOwnedDescriptor);
    CHECK_LIFECYCLE(factory.counters->closeCalls == 1);
    CHECK_LIFECYCLE(factory.openCalls == 1);
}

}  // namespace

int runTransportSessionLifecycleTest() {
    failures = 0;
    successfulOpenAndIdempotentCloseTest();
    failedOpenCleansPartialBackendTest();
    callerDescriptorRemainsBorrowedTest();
    return failures;
}

#include "transport/session.hpp"

#include <exception>
#include <new>
#include <utility>

namespace usbhost::transport {

std::shared_ptr<TransportSession> TransportSession::open(
        int borrowedFileDescriptor,
        AuthorizedBackendFactory &factory,
        TransportError &outError) {
    outError = {};
    if (borrowedFileDescriptor < 0) {
        outError = makeTransportError(BackendStatus::InvalidArgument,
                                      "authorized file descriptor is invalid");
        return nullptr;
    }

    std::unique_ptr<UsbBackend> backend;
    std::string diagnostic;
    BackendStatus status = BackendStatus::InternalFailure;
    try {
        status = factory.openAuthorizedFileDescriptor(
            borrowedFileDescriptor, backend, diagnostic);
    } catch (const std::exception &) {
        diagnostic = "backend factory failed";
    }
    if (status != BackendStatus::Success || !backend) {
        if (backend) {
            backend->close();
        }
        if (status == BackendStatus::Success) {
            status = BackendStatus::InternalFailure;
            diagnostic = "backend factory returned no backend";
        }
        outError = makeTransportError(status, diagnostic);
        return nullptr;
    }

    try {
        DeviceDescriptor descriptor = backend->deviceDescriptor();
        auto session = std::shared_ptr<TransportSession>(
            new TransportSession(std::move(backend), std::move(descriptor)));
        outError = makeTransportError(BackendStatus::Success, {});
        return session;
    } catch (const std::bad_alloc &) {
        if (backend) {
            backend->close();
        }
        outError = makeTransportError(BackendStatus::InternalFailure,
                                      "session allocation failed");
        return nullptr;
    }
}

TransportSession::TransportSession(std::unique_ptr<UsbBackend> backend,
                                   DeviceDescriptor descriptor)
    : backend_(std::move(backend)), descriptor_(std::move(descriptor)),
      state_(SessionState::Open) {}

TransportSession::~TransportSession() {
    close();
}

usbhost_status TransportSession::close() {
    std::unique_ptr<UsbBackend> backend;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (state_ == SessionState::Closed) {
            return USBHOST_OK;
        }
        if (state_ == SessionState::Closing) {
            closedCondition_.wait(lock, [this] { return state_ == SessionState::Closed; });
            return USBHOST_OK;
        }
        state_ = SessionState::Closing;
        backend = std::move(backend_);
    }

    if (backend) {
        backend->close();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = SessionState::Closed;
    }
    closedCondition_.notify_all();
    return USBHOST_OK;
}

SessionState TransportSession::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

const DeviceDescriptor &TransportSession::descriptorSnapshot() const noexcept {
    return descriptor_;
}

}  // namespace usbhost::transport

#ifndef USBHOST_TESTS_SCRIPTED_USB_BACKEND_HPP
#define USBHOST_TESTS_SCRIPTED_USB_BACKEND_HPP

#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "transport/backend.hpp"

namespace usbhost::transport {

struct ScriptedCompletion {
    TransferType transferType{TransferType::Control};
    BackendStatus status{BackendStatus::Success};
    std::vector<std::uint8_t> inputBytes;
    std::uint32_t actualLength{0};
    bool deferred{false};
    std::string diagnostic;
};

class ScriptedUsbBackend final : public UsbBackend {
public:
    const DeviceDescriptor &deviceDescriptor() const noexcept override {
        return descriptor_;
    }

    DeviceDescriptor &mutableDeviceDescriptor() noexcept {
        return descriptor_;
    }

    BackendStatus selectConfiguration(std::uint8_t) override {
        return closed_ ? BackendStatus::InternalFailure : BackendStatus::Success;
    }

    BackendStatus claimInterface(std::uint8_t) override {
        return closed_ ? BackendStatus::InternalFailure : BackendStatus::Success;
    }

    BackendStatus selectAlternateSetting(std::uint8_t, std::uint8_t) override {
        return closed_ ? BackendStatus::InternalFailure : BackendStatus::Success;
    }

    BackendStatus releaseInterface(std::uint8_t) override {
        return closed_ ? BackendStatus::InternalFailure : BackendStatus::Success;
    }

    void enqueue(ScriptedCompletion completion) {
        scripted_.push_back(std::move(completion));
    }

    OperationId submitControl(const ControlRequest &request,
                              MutableBufferView buffer,
                              CompletionCallback completion) override {
        return submit(TransferType::Control, request.direction, buffer, std::move(completion));
    }

    OperationId submitEndpoint(const EndpointTransferRequest &request,
                               MutableBufferView buffer,
                               CompletionCallback completion) override {
        return submit(request.transferType, request.direction, buffer, std::move(completion));
    }

    bool cancel(OperationId operation) override {
        if (!pending_ || pending_->id != operation) {
            return false;
        }
        pending_->script.status = BackendStatus::Cancelled;
        completePending();
        return true;
    }

    bool completeNext() {
        if (!pending_) {
            return false;
        }
        completePending();
        return true;
    }

    std::size_t pendingCount() const noexcept {
        return pending_ ? 1u : 0u;
    }

    std::size_t scriptedCount() const noexcept {
        return scripted_.size();
    }

    bool isClosed() const noexcept {
        return closed_;
    }

    void close() noexcept override {
        if (closed_) {
            return;
        }
        closed_ = true;
        if (pending_) {
            pending_->script.status = BackendStatus::Cancelled;
            completePending();
        }
        scripted_.clear();
    }

private:
    struct PendingOperation {
        OperationId id{kInvalidOperationId};
        Direction direction{Direction::Out};
        MutableBufferView buffer;
        CompletionCallback completion;
        ScriptedCompletion script;
    };

    OperationId submit(TransferType transferType,
                       Direction direction,
                       MutableBufferView buffer,
                       CompletionCallback completion) {
        if (!completion) {
            return kInvalidOperationId;
        }
        if (closed_) {
            completion({BackendStatus::InternalFailure, 0, "backend is closed"});
            return kInvalidOperationId;
        }
        if (!buffer.isValid()) {
            completion({BackendStatus::InvalidArgument, 0, "invalid buffer"});
            return kInvalidOperationId;
        }
        if (pending_) {
            completion({BackendStatus::Busy, 0, "operation already pending"});
            return kInvalidOperationId;
        }
        if (scripted_.empty()) {
            completion({BackendStatus::InternalFailure, 0, "no scripted completion"});
            return kInvalidOperationId;
        }

        ScriptedCompletion script = std::move(scripted_.front());
        scripted_.pop_front();
        if (script.transferType != transferType) {
            completion({BackendStatus::InvalidArgument, 0, "scripted transfer type mismatch"});
            return kInvalidOperationId;
        }

        const OperationId id = allocateOperationId();
        PendingOperation operation{id, direction, buffer, std::move(completion), std::move(script)};
        if (operation.script.deferred) {
            pending_ = std::move(operation);
        } else {
            complete(std::move(operation));
        }
        return id;
    }

    OperationId allocateOperationId() noexcept {
        const OperationId result = nextOperationId_;
        ++nextOperationId_;
        if (nextOperationId_ == kInvalidOperationId) {
            nextOperationId_ = UINT64_C(1);
        }
        return result;
    }

    void completePending() noexcept {
        PendingOperation operation = std::move(*pending_);
        pending_.reset();
        complete(std::move(operation));
    }

    static void complete(PendingOperation operation) noexcept {
        std::uint32_t actualLength = std::min(operation.script.actualLength,
                                              operation.buffer.capacity);
        if (operation.direction == Direction::In) {
            const auto availableInput = static_cast<std::uint32_t>(
                std::min(operation.script.inputBytes.size(),
                         static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())));
            actualLength = std::min(actualLength,
                                    availableInput);
            if (actualLength != 0) {
                std::copy_n(operation.script.inputBytes.data(), actualLength,
                            operation.buffer.data);
            }
        }
        CompletionCallback callback = std::move(operation.completion);
        try {
            callback({operation.script.status, actualLength,
                      std::move(operation.script.diagnostic)});
        } catch (...) {
            // Test callbacks are untrusted; close() must remain noexcept and deterministic.
        }
    }

    DeviceDescriptor descriptor_;
    std::deque<ScriptedCompletion> scripted_;
    std::optional<PendingOperation> pending_;
    OperationId nextOperationId_{UINT64_C(1)};
    bool closed_{false};
};

}  // namespace usbhost::transport

#endif

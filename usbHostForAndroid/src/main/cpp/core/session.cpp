#include "core/session.hpp"

#include <cstdint>
#include <utility>

namespace usbhost {

namespace {

bool containsRange(uint32_t regionBase, uint32_t regionSize,
                   uint32_t address, uint32_t length) {
    const uint64_t regionEnd = static_cast<uint64_t>(regionBase) + regionSize;
    const uint64_t requestEnd = static_cast<uint64_t>(address) + length;
    return address >= regionBase && requestEnd <= regionEnd;
}

}  // namespace

Session::Session(std::unique_ptr<Backend> backend) : backend_(std::move(backend)) {
    if (backend_) {
        programmer_ = backend_->programmerInfo();
    } else {
        state_ = SessionState::Failed;
    }
}

Session::~Session() {
    close();
}

usbhost_programmer_info Session::programmerInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return programmer_;
}

Result Session::connectTarget(usbhost_target_info &target) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == SessionState::TargetReady) {
        target = target_;
        return Result::ok();
    }
    if (state_ != SessionState::ProgrammerReady || !backend_) {
        return Result::error(USBHOST_INVALID_STATE, "session is not ready for target connection");
    }

    usbhost_target_info candidate = emptyTargetInfo();
    Result result = backend_->connectTarget(candidate);
    if (!result.isOk()) {
        failIfTerminal(result);
        return result;
    }
    if (candidate.chip_id != 0x467u) {
        return Result::error(USBHOST_UNSUPPORTED_TARGET,
                             "connected target is not STM32G0B/G0C (chip id 0x467)");
    }

    target_ = candidate;
    target_.struct_size = sizeof(target_);
    target = target_;
    state_ = SessionState::TargetReady;
    return Result::ok();
}

Result Session::readMemory(uint32_t address, uint8_t *destination, uint32_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SessionState::TargetReady || !backend_) {
        return Result::error(USBHOST_INVALID_STATE, "target is not connected");
    }
    if (destination == nullptr || !isReadableRange(address, length)) {
        return Result::error(USBHOST_INVALID_ARGUMENT,
                             "memory range must be non-empty, bounded, and inside flash or SRAM");
    }

    Result result = backend_->readMemory(address, destination, length);
    failIfTerminal(result);
    return result;
}

Result Session::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == SessionState::Closed) {
        return Result::ok();
    }
    if (backend_) {
        backend_->close();
        backend_.reset();
    }
    state_ = SessionState::Closed;
    return Result::ok();
}

SessionState Session::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

bool Session::isReadableRange(uint32_t address, uint32_t length) const {
    if (length == 0 || length > USBHOST_MAX_READ_SIZE) {
        return false;
    }
    const uint64_t requestEnd = static_cast<uint64_t>(address) + length;
    if (requestEnd > (static_cast<uint64_t>(UINT32_MAX) + 1u)) {
        return false;
    }
    return containsRange(target_.flash_base, target_.flash_size, address, length)
        || containsRange(target_.sram_base, target_.sram_size, address, length);
}

void Session::failIfTerminal(const Result &result) {
    if (result.status == USBHOST_DISCONNECTED) {
        state_ = SessionState::Failed;
    }
}

}  // namespace usbhost

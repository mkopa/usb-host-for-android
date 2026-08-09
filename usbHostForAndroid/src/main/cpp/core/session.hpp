#ifndef USBHOST_CORE_SESSION_HPP
#define USBHOST_CORE_SESSION_HPP

#include <memory>
#include <mutex>

#include "core/backend.hpp"

namespace usbhost {

class Session final {
public:
    explicit Session(std::unique_ptr<Backend> backend);
    ~Session();

    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    usbhost_programmer_info programmerInfo() const;
    Result connectTarget(usbhost_target_info &target);
    Result readMemory(uint32_t address, uint8_t *destination, uint32_t length);
    Result close();
    SessionState state() const;

private:
    bool isReadableRange(uint32_t address, uint32_t length) const;
    void failIfTerminal(const Result &result);

    mutable std::mutex mutex_;
    std::unique_ptr<Backend> backend_;
    SessionState state_{SessionState::ProgrammerReady};
    usbhost_programmer_info programmer_{};
    usbhost_target_info target_{};
};

}  // namespace usbhost

#endif

#ifndef USBHOST_CORE_TYPES_HPP
#define USBHOST_CORE_TYPES_HPP

#include <cstdint>
#include <string>
#include <utility>

#include "usbhost/usbhost.h"

namespace usbhost {

enum class SessionState {
    ProgrammerReady,
    TargetReady,
    Failed,
    Closed
};

struct Result {
    usbhost_status status{USBHOST_OK};
    std::string message;

    static Result ok() { return {}; }
    static Result error(usbhost_status value, std::string text) {
        return Result{value, std::move(text)};
    }
    bool isOk() const { return status == USBHOST_OK; }
};

inline usbhost_programmer_info emptyProgrammerInfo() {
    usbhost_programmer_info value{};
    value.struct_size = sizeof(value);
    return value;
}

inline usbhost_target_info emptyTargetInfo() {
    usbhost_target_info value{};
    value.struct_size = sizeof(value);
    value.target_voltage_mv = -1;
    return value;
}

}  // namespace usbhost

#endif

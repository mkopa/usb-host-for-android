#include "usbhost/usbhost.h"

#include <exception>
#include <memory>
#include <string>

#include "core/backend.hpp"
#include "core/registry.hpp"

namespace {

thread_local std::string lastError;
thread_local usbhost_status lastStatus = USBHOST_OK;

usbhost_status finish(const usbhost::Result &result) {
    lastStatus = result.status;
    lastError = result.message;
    return result.status;
}

usbhost_status invalid(const char *message) {
    lastStatus = USBHOST_INVALID_ARGUMENT;
    lastError = message;
    return USBHOST_INVALID_ARGUMENT;
}

bool validProgrammerOutput(const usbhost_programmer_info *value) {
    return value != nullptr && value->struct_size >= sizeof(usbhost_programmer_info);
}

bool validTargetOutput(const usbhost_target_info *value) {
    return value != nullptr && value->struct_size >= sizeof(usbhost_target_info);
}

}  // namespace

extern "C" {

uint32_t usbhost_abi_version(void) {
    return USBHOST_ABI_VERSION;
}

usbhost_status usbhost_open_stlink_v3_fd(
    int fd,
    uint16_t vendor_id,
    uint16_t product_id,
    uint32_t swd_frequency_khz,
    usbhost_session *out_session,
    usbhost_programmer_info *out_programmer) {
    lastError.clear();
    lastStatus = USBHOST_OK;
    if (fd < 0 || out_session == nullptr || !validProgrammerOutput(out_programmer)
            || swd_frequency_khz == 0 || swd_frequency_khz > 24000u) {
        return invalid("invalid descriptor, output, or SWD frequency");
    }
    *out_session = 0;

    try {
        usbhost::BackendOpenResult opened = usbhost::openStlinkBackend(
            fd, vendor_id, product_id, swd_frequency_khz);
        if (!opened.result.isOk() || !opened.backend) {
            return finish(opened.result.isOk()
                ? usbhost::Result::error(USBHOST_INTERNAL_ERROR, "backend returned no session")
                : opened.result);
        }
        auto session = std::make_shared<usbhost::Session>(std::move(opened.backend));
        *out_programmer = session->programmerInfo();
        *out_session = usbhost::registerSession(session);
        if (*out_session == 0) {
            session->close();
            return finish(usbhost::Result::error(USBHOST_INTERNAL_ERROR,
                                                 "could not allocate session handle"));
        }
        return finish(usbhost::Result::ok());
    } catch (const std::exception &error) {
        return finish(usbhost::Result::error(USBHOST_INTERNAL_ERROR, error.what()));
    } catch (...) {
        return finish(usbhost::Result::error(USBHOST_INTERNAL_ERROR,
                                             "unexpected native open failure"));
    }
}

usbhost_status usbhost_connect_target(usbhost_session session,
                                      usbhost_target_info *out_target) {
    lastError.clear();
    lastStatus = USBHOST_OK;
    if (session == 0 || !validTargetOutput(out_target)) {
        return invalid("invalid session or target output structure");
    }
    std::shared_ptr<usbhost::Session> found = usbhost::findSession(session);
    if (!found) {
        return finish(usbhost::Result::error(USBHOST_INVALID_STATE,
                                             "session is closed or unknown"));
    }
    return finish(found->connectTarget(*out_target));
}

usbhost_status usbhost_read_memory(usbhost_session session, uint32_t address,
                                   uint8_t *destination, uint32_t length) {
    lastError.clear();
    lastStatus = USBHOST_OK;
    if (session == 0 || destination == nullptr) {
        return invalid("invalid session or destination buffer");
    }
    std::shared_ptr<usbhost::Session> found = usbhost::findSession(session);
    if (!found) {
        return finish(usbhost::Result::error(USBHOST_INVALID_STATE,
                                             "session is closed or unknown"));
    }
    return finish(found->readMemory(address, destination, length));
}

usbhost_status usbhost_close(usbhost_session session) {
    lastError.clear();
    lastStatus = USBHOST_OK;
    if (session == 0) {
        return finish(usbhost::Result::ok());
    }
    std::shared_ptr<usbhost::Session> found = usbhost::retireSession(session);
    return found ? finish(found->close()) : finish(usbhost::Result::ok());
}

const char *usbhost_status_name(usbhost_status status) {
    switch (status) {
        case USBHOST_OK: return "OK";
        case USBHOST_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case USBHOST_PERMISSION_DENIED: return "PERMISSION_DENIED";
        case USBHOST_UNSUPPORTED_DEVICE: return "UNSUPPORTED_DEVICE";
        case USBHOST_USB_ERROR: return "USB_ERROR";
        case USBHOST_TIMEOUT: return "TIMEOUT";
        case USBHOST_DISCONNECTED: return "DISCONNECTED";
        case USBHOST_PROGRAMMER_ERROR: return "PROGRAMMER_ERROR";
        case USBHOST_TARGET_NOT_FOUND: return "TARGET_NOT_FOUND";
        case USBHOST_UNSUPPORTED_TARGET: return "UNSUPPORTED_TARGET";
        case USBHOST_INVALID_STATE: return "INVALID_STATE";
        case USBHOST_BUSY: return "BUSY";
        case USBHOST_INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: return "UNKNOWN_STATUS";
    }
}

usbhost_status usbhost_last_status(void) {
    return lastStatus;
}

const char *usbhost_last_error(void) {
    return lastError.c_str();
}

}  // extern "C"

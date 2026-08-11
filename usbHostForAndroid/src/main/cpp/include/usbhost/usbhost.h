#ifndef USBHOST_USBHOST_H
#define USBHOST_USBHOST_H

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__)
#define USBHOST_API __attribute__((visibility("default")))
#else
#define USBHOST_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define USBHOST_ABI_VERSION 1u
#define USBHOST_MAX_READ_SIZE (1024u * 1024u)

typedef uint64_t usbhost_session;

typedef enum usbhost_status {
    USBHOST_OK = 0,
    USBHOST_INVALID_ARGUMENT = 1,
    USBHOST_PERMISSION_DENIED = 2,
    USBHOST_UNSUPPORTED_DEVICE = 3,
    USBHOST_USB_ERROR = 4,
    USBHOST_TIMEOUT = 5,
    USBHOST_DISCONNECTED = 6,
    USBHOST_PROGRAMMER_ERROR = 7,
    USBHOST_TARGET_NOT_FOUND = 8,
    USBHOST_UNSUPPORTED_TARGET = 9,
    USBHOST_INVALID_STATE = 10,
    USBHOST_BUSY = 11,
    USBHOST_INTERNAL_ERROR = 12,
    USBHOST_STALL = 13,
    USBHOST_CANCELLED = 14,
    USBHOST_UNSUPPORTED_OPERATION = 15
} usbhost_status;

typedef struct usbhost_programmer_info {
    uint32_t struct_size;
    uint32_t stlink_version;
    uint32_t jtag_version;
    uint32_t swim_version;
    uint32_t jtag_api_version;
} usbhost_programmer_info;

typedef struct usbhost_target_info {
    uint32_t struct_size;
    uint32_t chip_id;
    uint32_t flash_base;
    uint32_t flash_size;
    uint32_t flash_page_size;
    uint32_t sram_base;
    uint32_t sram_size;
    int32_t target_voltage_mv;
} usbhost_target_info;

USBHOST_API uint32_t usbhost_abi_version(void);

USBHOST_API usbhost_status usbhost_open_stlink_v3_fd(
    int fd,
    uint16_t vendor_id,
    uint16_t product_id,
    uint32_t swd_frequency_khz,
    usbhost_session *out_session,
    usbhost_programmer_info *out_programmer);

USBHOST_API usbhost_status usbhost_connect_target(
    usbhost_session session,
    usbhost_target_info *out_target);

USBHOST_API usbhost_status usbhost_read_memory(
    usbhost_session session,
    uint32_t address,
    uint8_t *destination,
    uint32_t length);

USBHOST_API usbhost_status usbhost_close(usbhost_session session);

USBHOST_API const char *usbhost_status_name(usbhost_status status);
USBHOST_API usbhost_status usbhost_last_status(void);
USBHOST_API const char *usbhost_last_error(void);

#ifdef __cplusplus
}
#endif

#include "transport.h"

#endif

#include "usbhost/usbhost.h"

#include <cstring>

static_assert(USBHOST_OK == 0, "status ABI changed");
static_assert(USBHOST_INVALID_ARGUMENT == 1, "status ABI changed");
static_assert(USBHOST_PERMISSION_DENIED == 2, "status ABI changed");
static_assert(USBHOST_UNSUPPORTED_DEVICE == 3, "status ABI changed");
static_assert(USBHOST_USB_ERROR == 4, "status ABI changed");
static_assert(USBHOST_TIMEOUT == 5, "status ABI changed");
static_assert(USBHOST_DISCONNECTED == 6, "status ABI changed");
static_assert(USBHOST_PROGRAMMER_ERROR == 7, "status ABI changed");
static_assert(USBHOST_TARGET_NOT_FOUND == 8, "status ABI changed");
static_assert(USBHOST_UNSUPPORTED_TARGET == 9, "status ABI changed");
static_assert(USBHOST_INVALID_STATE == 10, "status ABI changed");
static_assert(USBHOST_BUSY == 11, "status ABI changed");
static_assert(USBHOST_INTERNAL_ERROR == 12, "status ABI changed");
static_assert(USBHOST_STALL == 13, "status ABI changed");
static_assert(USBHOST_CANCELLED == 14, "status ABI changed");
static_assert(USBHOST_UNSUPPORTED_OPERATION == 15, "status ABI changed");

int runStatusContractTest() {
    return std::strcmp(usbhost_status_name(USBHOST_STALL), "STALL") == 0
            && std::strcmp(usbhost_status_name(USBHOST_CANCELLED), "CANCELLED") == 0
            && std::strcmp(usbhost_status_name(USBHOST_UNSUPPORTED_OPERATION),
                           "UNSUPPORTED_OPERATION") == 0
        ? 0
        : 1;
}

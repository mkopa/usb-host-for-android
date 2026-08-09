#include <type_traits>

#include "usbhost/usbhost.h"

static_assert(std::is_standard_layout<usbhost_programmer_info>::value, "C ABI layout required");
static_assert(std::is_standard_layout<usbhost_target_info>::value, "C ABI layout required");

void usbhost_cxx_contract_compile() {
    auto *openFunction = &usbhost_open_stlink_v3_fd;
    auto *readFunction = &usbhost_read_memory;
    (void)openFunction;
    (void)readFunction;
}

#include "usbhost/usbhost.h"

_Static_assert(USBHOST_OK == 0, "status ABI changed");
_Static_assert(USBHOST_INTERNAL_ERROR == 12, "status ABI changed");
_Static_assert(sizeof(usbhost_session) == 8, "session ABI changed");

void usbhost_c_contract_compile(void) {
    usbhost_programmer_info programmer = {0};
    usbhost_target_info target = {0};
    programmer.struct_size = sizeof(programmer);
    target.struct_size = sizeof(target);
    (void)programmer;
    (void)target;
}

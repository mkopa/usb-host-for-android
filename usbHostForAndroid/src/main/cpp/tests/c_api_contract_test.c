#include "usbhost/usbhost.h"

_Static_assert(USBHOST_OK == 0, "status ABI changed");
_Static_assert(USBHOST_INTERNAL_ERROR == 12, "status ABI changed");
_Static_assert(USBHOST_UNSUPPORTED_OPERATION == 15, "status ABI changed");
_Static_assert(USBHOST_MAX_READ_SIZE == 1024u * 1024u, "read limit changed");
_Static_assert(sizeof(usbhost_session) == 8, "session ABI changed");

typedef usbhost_status (*open_stlink_signature)(
    int, uint16_t, uint16_t, uint32_t, usbhost_session *, usbhost_programmer_info *);
typedef usbhost_status (*connect_target_signature)(usbhost_session, usbhost_target_info *);
typedef usbhost_status (*read_memory_signature)(
    usbhost_session, uint32_t, uint8_t *, uint32_t);
typedef usbhost_status (*close_signature)(usbhost_session);

static open_stlink_signature const open_stlink = usbhost_open_stlink_v3_fd;
static connect_target_signature const connect_target = usbhost_connect_target;
static read_memory_signature const read_memory = usbhost_read_memory;
static close_signature const close_session = usbhost_close;

void usbhost_c_contract_compile(void) {
    usbhost_programmer_info programmer = {0};
    usbhost_target_info target = {0};
    programmer.struct_size = sizeof(programmer);
    target.struct_size = sizeof(target);
    (void)programmer; (void)target;
    (void)open_stlink; (void)connect_target; (void)read_memory; (void)close_session;
}

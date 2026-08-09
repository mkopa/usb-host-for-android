#include "stlink/stlink_usb_android.h"

static_assert(sizeof(usbhost_stlink_layout) == 3, "endpoint layout must remain byte-sized");

int runStlinkUsbContractTest() {
    usbhost_stlink_layout layout{};
    if (!usbhost_stlink_v3_layout(0x0483u, 0x374eu, &layout)) return 1;
    if (layout.request_endpoint != 0x01u || layout.reply_endpoint != 0x81u
            || layout.trace_endpoint != 0x82u) return 2;
    if (usbhost_stlink_v3_layout(0x0483u, 0x374du, &layout)) return 3;
    if (usbhost_stlink_v3_layout(0xffffu, 0x374eu, &layout)) return 4;
    if (usbhost_stlink_v3_layout(0x0483u, 0x374eu, nullptr)) return 5;
    return 0;
}

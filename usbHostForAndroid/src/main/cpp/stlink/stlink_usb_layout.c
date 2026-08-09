#include "stlink/stlink_usb_android.h"

#include <stddef.h>

#define ST_VID 0x0483u
#define STLINK_V3E 0x374eu
#define STLINK_V3S 0x374fu
#define STLINK_V3_2VCP 0x3753u
#define STLINK_V3_NO_MSD 0x3754u
#define STLINK_V3P 0x3757u

int usbhost_stlink_v3_layout(uint16_t vendor_id, uint16_t product_id,
                             usbhost_stlink_layout *layout) {
    if (layout == NULL || vendor_id != ST_VID) {
        return 0;
    }
    switch (product_id) {
        case STLINK_V3E:
        case STLINK_V3S:
        case STLINK_V3_2VCP:
        case STLINK_V3_NO_MSD:
        case STLINK_V3P:
            layout->request_endpoint = 0x01u;
            layout->reply_endpoint = 0x81u;
            layout->trace_endpoint = 0x82u;
            return 1;
        default:
            return 0;
    }
}

#ifndef USBHOST_STLINK_USB_ANDROID_H
#define USBHOST_STLINK_USB_ANDROID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usbhost_stlink_layout {
    uint8_t request_endpoint;
    uint8_t reply_endpoint;
    uint8_t trace_endpoint;
} usbhost_stlink_layout;

int usbhost_stlink_v3_layout(uint16_t vendor_id, uint16_t product_id,
                             usbhost_stlink_layout *layout);

#if defined(__ANDROID__)
struct _stlink;
typedef struct _stlink stlink_t;

stlink_t *usbhost_stlink_open_fd(int fd, uint16_t vendor_id, uint16_t product_id,
                                 int32_t swd_frequency_khz);
void usbhost_stlink_reset_transport_error(void);
int usbhost_stlink_last_transport_error(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

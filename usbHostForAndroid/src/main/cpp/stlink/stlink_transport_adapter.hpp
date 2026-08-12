#ifndef USBHOST_STLINK_TRANSPORT_ADAPTER_HPP
#define USBHOST_STLINK_TRANSPORT_ADAPTER_HPP

#include <stdint.h>

#include "stlink/stlink_usb_android.h"
#include "usbhost/usbhost.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usbhost_stlink_transport_hooks {
    void *context;
    usbhost_status (*claim_interface)(void *context, uint8_t interface_number);
    usbhost_status (*bulk_transfer)(void *context, uint8_t endpoint_address,
                                    uint8_t *buffer, uint32_t length,
                                    uint32_t timeout_ms, uint32_t *out_actual_length);
    usbhost_status (*release_interface)(void *context, uint8_t interface_number);
    void (*close)(void *context);
} usbhost_stlink_transport_hooks;

typedef struct usbhost_stlink_transport_adapter usbhost_stlink_transport_adapter;

usbhost_stlink_transport_adapter *usbhost_stlink_transport_adapter_create(
    const usbhost_stlink_transport_hooks *hooks,
    const usbhost_stlink_layout *layout);
usbhost_stlink_transport_adapter *usbhost_stlink_transport_adapter_open(
    const usbhost_stlink_transport_hooks *hooks,
    const usbhost_stlink_layout *layout,
    usbhost_status *out_status);
usbhost_status usbhost_stlink_transport_adapter_claim(
    usbhost_stlink_transport_adapter *adapter);
usbhost_status usbhost_stlink_transport_adapter_bulk(
    usbhost_stlink_transport_adapter *adapter,
    uint8_t endpoint_address,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);
void usbhost_stlink_transport_adapter_close(usbhost_stlink_transport_adapter *adapter);
void usbhost_stlink_transport_adapter_destroy(usbhost_stlink_transport_adapter *adapter);

#if defined(__ANDROID__)
stlink_t *usbhost_stlink_open_transport(
    const usbhost_stlink_transport_hooks *hooks,
    const usbhost_stlink_layout *layout,
    int32_t swd_frequency_khz,
    const char *serial);
#endif

#ifdef __cplusplus
}
#endif

#endif

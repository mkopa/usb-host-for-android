#ifndef USBHOST_STLINK_SHARED_TRANSPORT_HPP
#define USBHOST_STLINK_SHARED_TRANSPORT_HPP

#include "stlink/stlink_transport_adapter.hpp"
#include "usbhost/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usbhost_stlink_shared_transport_api {
    void *context;
    usbhost_status (*open_fd)(void *context, int borrowed_fd,
                              usbhost_transport_session *out_session);
    usbhost_status (*claim_interface)(void *context, usbhost_transport_session session,
                                      uint8_t interface_number);
    usbhost_status (*bulk_transfer)(void *context, usbhost_transport_session session,
                                    uint8_t endpoint_address, uint8_t *buffer,
                                    uint32_t length, uint32_t timeout_ms,
                                    uint32_t *out_actual_length);
    usbhost_status (*release_interface)(void *context, usbhost_transport_session session,
                                        uint8_t interface_number);
    usbhost_status (*close)(void *context, usbhost_transport_session session);
} usbhost_stlink_shared_transport_api;

usbhost_stlink_shared_transport_api usbhost_stlink_production_transport_api(void);

/**
 * Opens a generic transport session from a borrowed authorized descriptor.
 * The resulting hooks own only that native session; the caller retains its Android connection.
 */
usbhost_status usbhost_stlink_open_shared_transport(
    int borrowed_fd,
    const usbhost_stlink_shared_transport_api *api,
    usbhost_stlink_transport_hooks *out_hooks);

#ifdef __cplusplus
}
#endif

#endif

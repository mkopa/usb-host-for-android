#include <cstddef>
#include <cstdint>

#include "usbhost/transport.h"

static_assert(USBHOST_TRANSPORT_DIRECTION_OUT == 0 && USBHOST_TRANSPORT_DIRECTION_IN == 1);
static_assert(USBHOST_TRANSPORT_TRANSFER_CONTROL == 0);
static_assert(USBHOST_TRANSPORT_TRANSFER_ISOCHRONOUS == 1);
static_assert(USBHOST_TRANSPORT_TRANSFER_BULK == 2);
static_assert(USBHOST_TRANSPORT_TRANSFER_INTERRUPT == 3);
static_assert(USBHOST_TRANSPORT_DESCRIPTOR_CONFIGURATION == 1);
static_assert(USBHOST_TRANSPORT_DESCRIPTOR_ALTERNATE_SETTING == 2);
static_assert(USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT == 3);
static_assert(sizeof(usbhost_transport_device_descriptor) == 40);
static_assert(sizeof(usbhost_transport_configuration_descriptor) == 32);
static_assert(sizeof(usbhost_transport_interface_descriptor) == 24);
static_assert(sizeof(usbhost_transport_alternate_setting_descriptor) == 32);
static_assert(sizeof(usbhost_transport_endpoint_descriptor) == 32);
static_assert(sizeof(usbhost_transport_descriptor_location) == 40);
#define PREFIX(type) static_assert(offsetof(type, struct_size) == 0)
PREFIX(usbhost_transport_device_descriptor);
PREFIX(usbhost_transport_configuration_descriptor);
PREFIX(usbhost_transport_interface_descriptor);
PREFIX(usbhost_transport_alternate_setting_descriptor);
PREFIX(usbhost_transport_endpoint_descriptor);
PREFIX(usbhost_transport_descriptor_location);

int runTransportCompatibilityTest() {
    const void *symbols[] = {
        reinterpret_cast<const void *>(&usbhost_transport_open_fd),
        reinterpret_cast<const void *>(&usbhost_transport_cancel),
        reinterpret_cast<const void *>(&usbhost_transport_close),
        reinterpret_cast<const void *>(&usbhost_transport_get_device_descriptor),
        reinterpret_cast<const void *>(&usbhost_transport_get_configuration_count),
        reinterpret_cast<const void *>(&usbhost_transport_get_configuration_at),
        reinterpret_cast<const void *>(&usbhost_transport_get_interface_count),
        reinterpret_cast<const void *>(&usbhost_transport_get_interface_at),
        reinterpret_cast<const void *>(&usbhost_transport_get_alternate_setting_count),
        reinterpret_cast<const void *>(&usbhost_transport_get_alternate_setting_at),
        reinterpret_cast<const void *>(&usbhost_transport_get_endpoint_count),
        reinterpret_cast<const void *>(&usbhost_transport_get_endpoint_at),
        reinterpret_cast<const void *>(&usbhost_transport_get_additional_descriptor_at),
        reinterpret_cast<const void *>(&usbhost_transport_select_configuration),
        reinterpret_cast<const void *>(&usbhost_transport_claim_interface),
        reinterpret_cast<const void *>(&usbhost_transport_select_alternate_setting),
        reinterpret_cast<const void *>(&usbhost_transport_release_interface),
        reinterpret_cast<const void *>(&usbhost_transport_control_transfer),
        reinterpret_cast<const void *>(&usbhost_transport_bulk_transfer),
        reinterpret_cast<const void *>(&usbhost_transport_interrupt_transfer)};
    for (const void *symbol : symbols) if (!symbol) return 1;
    return 0;
}

#include <stddef.h>
#include <stdint.h>

#include "usbhost/transport.h"

_Static_assert(sizeof(usbhost_transport_session) == sizeof(uint64_t),
               "transport session width changed");
_Static_assert(USBHOST_TRANSPORT_INVALID_SESSION == UINT64_C(0),
               "invalid session value changed");
_Static_assert(USBHOST_TRANSPORT_MAX_CONTROL_LENGTH == UINT32_C(65535),
               "control limit changed");
_Static_assert(USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH == UINT32_C(1048576),
               "endpoint limit changed");
_Static_assert(USBHOST_TRANSPORT_MIN_TIMEOUT_MS == UINT32_C(1),
               "minimum timeout changed");
_Static_assert(USBHOST_TRANSPORT_MAX_TIMEOUT_MS == UINT32_C(60000),
               "maximum timeout changed");
_Static_assert(USBHOST_TRANSPORT_MAX_ADDITIONAL_DESCRIPTOR_LENGTH == UINT32_C(65535),
               "additional descriptor limit changed");

#define ASSERT_SIZE_VERSIONED(type) \
    _Static_assert(offsetof(type, struct_size) == 0, #type " must start with struct_size")

ASSERT_SIZE_VERSIONED(usbhost_transport_device_descriptor);
ASSERT_SIZE_VERSIONED(usbhost_transport_configuration_descriptor);
ASSERT_SIZE_VERSIONED(usbhost_transport_interface_descriptor);
ASSERT_SIZE_VERSIONED(usbhost_transport_alternate_setting_descriptor);
ASSERT_SIZE_VERSIONED(usbhost_transport_endpoint_descriptor);
ASSERT_SIZE_VERSIONED(usbhost_transport_descriptor_location);

_Static_assert(sizeof(((usbhost_transport_device_descriptor *)0)->snapshot_generation)
                   == sizeof(uint64_t),
               "snapshot generation width changed");
_Static_assert(sizeof(((usbhost_transport_endpoint_descriptor *)0)->maximum_packet_size)
                   == sizeof(uint16_t),
               "packet size width changed");

void usbhost_transport_c_header_contract_compile(void) {
    usbhost_transport_device_descriptor device = {0};
    usbhost_transport_configuration_descriptor configuration = {0};
    usbhost_transport_interface_descriptor interface_value = {0};
    usbhost_transport_alternate_setting_descriptor alternate = {0};
    usbhost_transport_endpoint_descriptor endpoint = {0};
    usbhost_transport_descriptor_location location = {0};
    device.struct_size = (uint32_t)sizeof(device);
    configuration.struct_size = (uint32_t)sizeof(configuration);
    interface_value.struct_size = (uint32_t)sizeof(interface_value);
    alternate.struct_size = (uint32_t)sizeof(alternate);
    endpoint.struct_size = (uint32_t)sizeof(endpoint);
    location.struct_size = (uint32_t)sizeof(location);

    (void)sizeof(&usbhost_transport_open_fd);
    (void)sizeof(&usbhost_transport_cancel);
    (void)sizeof(&usbhost_transport_close);
    (void)sizeof(&usbhost_transport_get_device_descriptor);
    (void)sizeof(&usbhost_transport_get_configuration_count);
    (void)sizeof(&usbhost_transport_get_configuration_at);
    (void)sizeof(&usbhost_transport_get_interface_count);
    (void)sizeof(&usbhost_transport_get_interface_at);
    (void)sizeof(&usbhost_transport_get_alternate_setting_count);
    (void)sizeof(&usbhost_transport_get_alternate_setting_at);
    (void)sizeof(&usbhost_transport_get_endpoint_count);
    (void)sizeof(&usbhost_transport_get_endpoint_at);
    (void)sizeof(&usbhost_transport_get_additional_descriptor_at);
    (void)sizeof(&usbhost_transport_select_configuration);
    (void)sizeof(&usbhost_transport_claim_interface);
    (void)sizeof(&usbhost_transport_select_alternate_setting);
    (void)sizeof(&usbhost_transport_release_interface);
    (void)sizeof(&usbhost_transport_control_transfer);
    (void)sizeof(&usbhost_transport_bulk_transfer);
    (void)sizeof(&usbhost_transport_interrupt_transfer);
}

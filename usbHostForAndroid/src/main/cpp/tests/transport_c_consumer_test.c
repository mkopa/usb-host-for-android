#include <stdint.h>

#include "usbhost/transport.h"

#define USE_TRANSPORT_FUNCTION(name) do { if (&name == 0) return 1; } while (0)

int runTransportCConsumerTest(void) {
    usbhost_transport_device_descriptor device = {0};
    usbhost_transport_configuration_descriptor configuration = {0};
    usbhost_transport_interface_descriptor interface_value = {0};
    usbhost_transport_alternate_setting_descriptor alternate = {0};
    usbhost_transport_endpoint_descriptor endpoint = {0};
    usbhost_transport_descriptor_location location = {0};
    device.struct_size = sizeof(device);
    configuration.struct_size = sizeof(configuration);
    interface_value.struct_size = sizeof(interface_value);
    alternate.struct_size = sizeof(alternate);
    endpoint.struct_size = sizeof(endpoint);
    location.struct_size = sizeof(location);

    USE_TRANSPORT_FUNCTION(usbhost_transport_open_fd);
    USE_TRANSPORT_FUNCTION(usbhost_transport_cancel);
    USE_TRANSPORT_FUNCTION(usbhost_transport_close);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_device_descriptor);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_configuration_count);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_configuration_at);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_interface_count);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_interface_at);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_alternate_setting_count);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_alternate_setting_at);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_endpoint_count);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_endpoint_at);
    USE_TRANSPORT_FUNCTION(usbhost_transport_get_additional_descriptor_at);
    USE_TRANSPORT_FUNCTION(usbhost_transport_select_configuration);
    USE_TRANSPORT_FUNCTION(usbhost_transport_claim_interface);
    USE_TRANSPORT_FUNCTION(usbhost_transport_select_alternate_setting);
    USE_TRANSPORT_FUNCTION(usbhost_transport_release_interface);
    USE_TRANSPORT_FUNCTION(usbhost_transport_control_transfer);
    USE_TRANSPORT_FUNCTION(usbhost_transport_bulk_transfer);
    USE_TRANSPORT_FUNCTION(usbhost_transport_interrupt_transfer);

    return device.struct_size != 0 && configuration.struct_size != 0
            && interface_value.struct_size != 0 && alternate.struct_size != 0
            && endpoint.struct_size != 0 && location.struct_size != 0 ? 0 : 1;
}

#include <cstdint>
#include <type_traits>

#include "usbhost/transport.h"

namespace {
template <typename Function>
bool exported(Function function) noexcept { return function != nullptr; }
}

int runTransportCxxConsumerTest() {
    static_assert(std::is_standard_layout_v<usbhost_transport_device_descriptor>);
    static_assert(std::is_standard_layout_v<usbhost_transport_configuration_descriptor>);
    static_assert(std::is_standard_layout_v<usbhost_transport_interface_descriptor>);
    static_assert(std::is_standard_layout_v<usbhost_transport_alternate_setting_descriptor>);
    static_assert(std::is_standard_layout_v<usbhost_transport_endpoint_descriptor>);
    static_assert(std::is_standard_layout_v<usbhost_transport_descriptor_location>);
    static_assert(std::is_same_v<usbhost_transport_session, std::uint64_t>);

    usbhost_transport_device_descriptor device{sizeof(device)};
    usbhost_transport_configuration_descriptor configuration{sizeof(configuration)};
    usbhost_transport_interface_descriptor interfaceValue{sizeof(interfaceValue)};
    usbhost_transport_alternate_setting_descriptor alternate{sizeof(alternate)};
    usbhost_transport_endpoint_descriptor endpoint{sizeof(endpoint)};
    usbhost_transport_descriptor_location location{sizeof(location)};

    const bool functions = exported(&usbhost_transport_open_fd)
        && exported(&usbhost_transport_cancel) && exported(&usbhost_transport_close)
        && exported(&usbhost_transport_get_device_descriptor)
        && exported(&usbhost_transport_get_configuration_count)
        && exported(&usbhost_transport_get_configuration_at)
        && exported(&usbhost_transport_get_interface_count)
        && exported(&usbhost_transport_get_interface_at)
        && exported(&usbhost_transport_get_alternate_setting_count)
        && exported(&usbhost_transport_get_alternate_setting_at)
        && exported(&usbhost_transport_get_endpoint_count)
        && exported(&usbhost_transport_get_endpoint_at)
        && exported(&usbhost_transport_get_additional_descriptor_at)
        && exported(&usbhost_transport_select_configuration)
        && exported(&usbhost_transport_claim_interface)
        && exported(&usbhost_transport_select_alternate_setting)
        && exported(&usbhost_transport_release_interface)
        && exported(&usbhost_transport_control_transfer)
        && exported(&usbhost_transport_bulk_transfer)
        && exported(&usbhost_transport_interrupt_transfer);
    const bool records = device.struct_size && configuration.struct_size
        && interfaceValue.struct_size && alternate.struct_size
        && endpoint.struct_size && location.struct_size;
    return functions && records ? 0 : 1;
}

#include <cstdint>
#include <type_traits>

#include "usbhost/transport.h"

static_assert(std::is_same_v<usbhost_transport_session, std::uint64_t>);
static_assert(std::is_standard_layout_v<usbhost_transport_device_descriptor>);
static_assert(std::is_standard_layout_v<usbhost_transport_configuration_descriptor>);
static_assert(std::is_standard_layout_v<usbhost_transport_interface_descriptor>);
static_assert(std::is_standard_layout_v<usbhost_transport_alternate_setting_descriptor>);
static_assert(std::is_standard_layout_v<usbhost_transport_endpoint_descriptor>);
static_assert(std::is_standard_layout_v<usbhost_transport_descriptor_location>);

using OpenFunction = usbhost_status (*)(int, usbhost_transport_session *);
using CloseFunction = usbhost_status (*)(usbhost_transport_session);
using CountFunction = usbhost_status (*)(usbhost_transport_session, std::uint32_t *);
using ControlFunction = usbhost_status (*)(usbhost_transport_session, std::uint8_t,
                                            std::uint8_t, std::uint16_t, std::uint16_t,
                                            std::uint8_t *, std::uint32_t, std::uint32_t,
                                            std::uint32_t *);
using EndpointFunction = usbhost_status (*)(usbhost_transport_session, std::uint8_t,
                                             std::uint8_t *, std::uint32_t, std::uint32_t,
                                             std::uint32_t *);

static_assert(std::is_same_v<decltype(&usbhost_transport_open_fd), OpenFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_cancel), CloseFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_close), CloseFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_get_configuration_count), CountFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_control_transfer), ControlFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_bulk_transfer), EndpointFunction>);
static_assert(std::is_same_v<decltype(&usbhost_transport_interrupt_transfer), EndpointFunction>);

static_assert(noexcept(static_cast<void>(USBHOST_TRANSPORT_MAX_CONTROL_LENGTH)));

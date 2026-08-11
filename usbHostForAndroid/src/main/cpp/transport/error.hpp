#ifndef USBHOST_TRANSPORT_ERROR_HPP
#define USBHOST_TRANSPORT_ERROR_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "usbhost/usbhost.h"

namespace usbhost::transport {

constexpr std::size_t kMaximumDiagnosticLength = 240;

enum class BackendStatus : std::uint8_t {
    Success,
    InvalidArgument,
    PermissionDenied,
    UnsupportedDevice,
    UnsupportedOperation,
    Busy,
    Timeout,
    Stall,
    Cancelled,
    Disconnected,
    UsbFailure,
    InternalFailure
};

struct TransportError {
    usbhost_status status{USBHOST_OK};
    std::uint32_t actualLength{0};
    std::string diagnostic;
};

usbhost_status mapBackendStatus(BackendStatus status) noexcept;
std::string sanitizeDiagnostic(std::string_view input);
TransportError makeTransportError(BackendStatus status,
                                  std::string_view diagnostic,
                                  std::uint32_t actualLength = 0);

}  // namespace usbhost::transport

#endif

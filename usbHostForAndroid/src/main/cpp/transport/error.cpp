#include "transport/error.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace usbhost::transport {
namespace {

constexpr std::size_t kMaximumDiagnosticInputLength = 4096;
constexpr std::string_view kRedacted = "[redacted]";

std::string lowercase(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

bool tokenTerminator(unsigned char character) {
    return std::isspace(character) || character == ',' || character == ';'
        || character == ']' || character == ')';
}

void redactLabeledValues(std::string &text) {
    constexpr std::array<std::string_view, 11> keys{{
        "serial=", "serial:", "serial_number=", "device_id=", "device-id=",
        "path=", "usb_path=", "usb-path=", "pointer=", "private=",
        "raw_descriptor="
    }};

    for (const std::string_view key : keys) {
        std::string folded = lowercase(text);
        std::size_t searchFrom = 0;
        while (true) {
            const std::size_t keyPosition = folded.find(key, searchFrom);
            if (keyPosition == std::string::npos) {
                break;
            }
            std::size_t valueStart = keyPosition + key.size();
            while (valueStart < text.size()
                    && std::isspace(static_cast<unsigned char>(text[valueStart]))) {
                ++valueStart;
            }
            std::size_t valueEnd = valueStart;
            while (valueEnd < text.size()
                    && !tokenTerminator(static_cast<unsigned char>(text[valueEnd]))) {
                ++valueEnd;
            }
            if (valueEnd > valueStart) {
                text.replace(valueStart, valueEnd - valueStart, kRedacted);
                folded = lowercase(text);
                searchFrom = valueStart + kRedacted.size();
            } else {
                searchFrom = valueStart;
            }
        }
    }
}

void redactUsbFilesystemPaths(std::string &text) {
    constexpr std::string_view prefix = "/dev/bus/usb/";
    std::string folded = lowercase(text);
    std::size_t searchFrom = 0;
    while (true) {
        const std::size_t pathStart = folded.find(prefix, searchFrom);
        if (pathStart == std::string::npos) {
            break;
        }
        std::size_t pathEnd = pathStart;
        while (pathEnd < text.size()
                && !tokenTerminator(static_cast<unsigned char>(text[pathEnd]))) {
            ++pathEnd;
        }
        text.replace(pathStart, pathEnd - pathStart, "[usb-path-redacted]");
        folded = lowercase(text);
        searchFrom = pathStart + sizeof("[usb-path-redacted]") - 1;
    }
}

}  // namespace

usbhost_status mapBackendStatus(BackendStatus status) noexcept {
    switch (status) {
        case BackendStatus::Success: return USBHOST_OK;
        case BackendStatus::InvalidArgument: return USBHOST_INVALID_ARGUMENT;
        case BackendStatus::PermissionDenied: return USBHOST_PERMISSION_DENIED;
        case BackendStatus::UnsupportedDevice: return USBHOST_UNSUPPORTED_DEVICE;
        case BackendStatus::UnsupportedOperation: return USBHOST_UNSUPPORTED_OPERATION;
        case BackendStatus::Busy: return USBHOST_BUSY;
        case BackendStatus::Timeout: return USBHOST_TIMEOUT;
        case BackendStatus::Stall: return USBHOST_STALL;
        case BackendStatus::Cancelled: return USBHOST_CANCELLED;
        case BackendStatus::Disconnected: return USBHOST_DISCONNECTED;
        case BackendStatus::UsbFailure: return USBHOST_USB_ERROR;
        case BackendStatus::InternalFailure: return USBHOST_INTERNAL_ERROR;
    }
    return USBHOST_INTERNAL_ERROR;
}

std::string sanitizeDiagnostic(std::string_view input) {
    const std::size_t inspectedLength = std::min(input.size(), kMaximumDiagnosticInputLength);
    std::string result(input.substr(0, inspectedLength));
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return std::iscntrl(character) ? ' ' : static_cast<char>(character);
    });
    redactLabeledValues(result);
    redactUsbFilesystemPaths(result);
    if (result.size() > kMaximumDiagnosticLength) {
        result.resize(kMaximumDiagnosticLength - 3);
        result.append("...");
    }
    return result;
}

TransportError makeTransportError(BackendStatus status,
                                  std::string_view diagnostic,
                                  std::uint32_t actualLength) {
    return TransportError{mapBackendStatus(status), actualLength,
                          sanitizeDiagnostic(diagnostic)};
}

}  // namespace usbhost::transport

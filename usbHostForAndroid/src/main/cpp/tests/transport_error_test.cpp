#include <array>
#include <string>

#include "transport/error.hpp"

namespace {

int failures = 0;

#define CHECK_ERROR(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

void mappingTest() {
    using usbhost::transport::BackendStatus;
    using usbhost::transport::mapBackendStatus;
    const std::array<std::pair<BackendStatus, usbhost_status>, 12> mappings{{
        {BackendStatus::Success, USBHOST_OK},
        {BackendStatus::InvalidArgument, USBHOST_INVALID_ARGUMENT},
        {BackendStatus::PermissionDenied, USBHOST_PERMISSION_DENIED},
        {BackendStatus::UnsupportedDevice, USBHOST_UNSUPPORTED_DEVICE},
        {BackendStatus::UnsupportedOperation, USBHOST_UNSUPPORTED_OPERATION},
        {BackendStatus::Busy, USBHOST_BUSY},
        {BackendStatus::Timeout, USBHOST_TIMEOUT},
        {BackendStatus::Stall, USBHOST_STALL},
        {BackendStatus::Cancelled, USBHOST_CANCELLED},
        {BackendStatus::Disconnected, USBHOST_DISCONNECTED},
        {BackendStatus::UsbFailure, USBHOST_USB_ERROR},
        {BackendStatus::InternalFailure, USBHOST_INTERNAL_ERROR}
    }};
    for (const auto &mapping : mappings) {
        CHECK_ERROR(mapBackendStatus(mapping.first) == mapping.second);
    }
}

void redactionTest() {
    using usbhost::transport::sanitizeDiagnostic;
    const std::string diagnostic = sanitizeDiagnostic(
        "transfer failed\nserial=ABC123 path=/dev/bus/usb/001/002 "
        "pointer=0x1234 private=payload device_id=42 code=-7");
    CHECK_ERROR(diagnostic.find("transfer failed") != std::string::npos);
    CHECK_ERROR(diagnostic.find("code=-7") != std::string::npos);
    CHECK_ERROR(diagnostic.find("ABC123") == std::string::npos);
    CHECK_ERROR(diagnostic.find("/dev/bus/usb") == std::string::npos);
    CHECK_ERROR(diagnostic.find("0x1234") == std::string::npos);
    CHECK_ERROR(diagnostic.find("payload") == std::string::npos);
    CHECK_ERROR(diagnostic.find("device_id=42") == std::string::npos);
    CHECK_ERROR(diagnostic.find('\n') == std::string::npos);
}

void unlabeledPathAndBoundTest() {
    using namespace usbhost::transport;
    const std::string path = sanitizeDiagnostic("detached at /dev/bus/usb/999/888 safely");
    CHECK_ERROR(path.find("detached at") != std::string::npos);
    CHECK_ERROR(path.find("/dev/bus/usb") == std::string::npos);

    const std::string bounded = sanitizeDiagnostic(std::string(1000, 'x'));
    CHECK_ERROR(bounded.size() <= kMaximumDiagnosticLength);
    const TransportError error = makeTransportError(
        BackendStatus::Cancelled, "serial=hidden cancellation", 19);
    CHECK_ERROR(error.status == USBHOST_CANCELLED);
    CHECK_ERROR(error.actualLength == 19);
    CHECK_ERROR(error.diagnostic.find("hidden") == std::string::npos);
}

}  // namespace

int runTransportErrorTest() {
    failures = 0;
    mappingTest();
    redactionTest();
    unlabeledPathAndBoundTest();
    return failures;
}

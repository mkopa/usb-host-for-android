#include "android/android_usb_backend.hpp"

#include <cstdint>
#include <new>
#include <utility>
#include <vector>

#if defined(ANDROID)
#include <unistd.h>
#include "libusb.h"
#endif

namespace usbhost::android {

namespace {

using transport::BackendStatus;

#if defined(ANDROID)
BackendStatus mapLibusb(int result) noexcept {
    switch (result) {
        case LIBUSB_SUCCESS: return BackendStatus::Success;
        case LIBUSB_ERROR_INVALID_PARAM: return BackendStatus::InvalidArgument;
        case LIBUSB_ERROR_ACCESS: return BackendStatus::PermissionDenied;
        case LIBUSB_ERROR_BUSY: return BackendStatus::Busy;
        case LIBUSB_ERROR_TIMEOUT: return BackendStatus::Timeout;
        case LIBUSB_ERROR_PIPE: return BackendStatus::Stall;
        case LIBUSB_ERROR_INTERRUPTED: return BackendStatus::Cancelled;
        case LIBUSB_ERROR_NO_DEVICE:
        case LIBUSB_ERROR_NOT_FOUND: return BackendStatus::Disconnected;
        case LIBUSB_ERROR_NOT_SUPPORTED: return BackendStatus::UnsupportedOperation;
        default: return BackendStatus::UsbFailure;
    }
}

void append16(std::vector<std::uint8_t> &output, std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>(value & 0xff));
    output.push_back(static_cast<std::uint8_t>(value >> 8));
}

bool appendExtra(std::vector<std::uint8_t> &output,
                 const unsigned char *extra, int length) {
    if (length < 0 || (length != 0 && extra == nullptr) ||
        output.size() + static_cast<std::size_t>(length) > UINT16_MAX) {
        return false;
    }
    if (length == 0) {
        return true;
    }
    output.insert(output.end(), extra, extra + length);
    return true;
}

int productionDuplicateFd(void *, int borrowedFd) { return dup(borrowedFd); }
void productionCloseFd(void *, int ownedFd) { close(ownedFd); }

BackendStatus productionAcquireRuntime(
        void *, AndroidUsbRuntimeLease &outLease, std::string &diagnostic) {
    int error = 0;
    auto runtime = acquireLibusbRuntime(error);
    if (!runtime) {
        diagnostic = "libusb runtime initialization failed";
        return mapLibusb(error);
    }
    outLease.context = runtime->nativeContext();
    outLease.owner = std::move(runtime);
    return BackendStatus::Success;
}

BackendStatus productionWrapDevice(
        void *, LibusbContext context, int ownedFd, void **outHandle,
        std::string &diagnostic) {
    libusb_device_handle *handle = nullptr;
    const int result = libusb_wrap_sys_device(
        static_cast<libusb_context *>(context), static_cast<intptr_t>(ownedFd), &handle);
    if (result != LIBUSB_SUCCESS) {
        diagnostic = "authorized USB handle wrapping failed";
        return mapLibusb(result);
    }
    *outHandle = handle;
    return BackendStatus::Success;
}

BackendStatus productionExtractDescriptors(
        void *, void *opaqueHandle, transport::RawDescriptorSet &out,
        std::string &diagnostic) {
    auto *handle = static_cast<libusb_device_handle *>(opaqueHandle);
    libusb_device *device = libusb_get_device(handle);
    libusb_device_descriptor deviceDescriptor{};
    int result = libusb_get_device_descriptor(device, &deviceDescriptor);
    if (result != LIBUSB_SUCCESS) {
        diagnostic = "device descriptor read failed";
        return mapLibusb(result);
    }
    int activeConfiguration = 0;
    result = libusb_get_configuration(handle, &activeConfiguration);
    if (result != LIBUSB_SUCCESS) {
        diagnostic = "active configuration read failed";
        return mapLibusb(result);
    }

    transport::RawDescriptorSet candidate;
    candidate.deviceDescriptor = {
        18, LIBUSB_DT_DEVICE,
        static_cast<std::uint8_t>(deviceDescriptor.bcdUSB & 0xff),
        static_cast<std::uint8_t>(deviceDescriptor.bcdUSB >> 8),
        deviceDescriptor.bDeviceClass, deviceDescriptor.bDeviceSubClass,
        deviceDescriptor.bDeviceProtocol, deviceDescriptor.bMaxPacketSize0,
        static_cast<std::uint8_t>(deviceDescriptor.idVendor & 0xff),
        static_cast<std::uint8_t>(deviceDescriptor.idVendor >> 8),
        static_cast<std::uint8_t>(deviceDescriptor.idProduct & 0xff),
        static_cast<std::uint8_t>(deviceDescriptor.idProduct >> 8),
        static_cast<std::uint8_t>(deviceDescriptor.bcdDevice & 0xff),
        static_cast<std::uint8_t>(deviceDescriptor.bcdDevice >> 8),
        0, 0, 0, deviceDescriptor.bNumConfigurations};
    candidate.activeConfigurationValue = static_cast<std::uint8_t>(activeConfiguration);
    candidate.generation = transport::SnapshotGeneration::initial();

    for (std::uint8_t configurationIndex = 0;
            configurationIndex < deviceDescriptor.bNumConfigurations;
            ++configurationIndex) {
        libusb_config_descriptor *configuration = nullptr;
        result = libusb_get_config_descriptor(device, configurationIndex, &configuration);
        if (result != LIBUSB_SUCCESS || configuration == nullptr) {
            diagnostic = "configuration descriptor read failed";
            return mapLibusb(result == LIBUSB_SUCCESS ? LIBUSB_ERROR_OTHER : result);
        }
        std::vector<std::uint8_t> raw = {
            9, LIBUSB_DT_CONFIG, 0, 0, configuration->bNumInterfaces,
            configuration->bConfigurationValue, 0, configuration->bmAttributes,
            configuration->MaxPower};
        bool valid = appendExtra(raw, configuration->extra, configuration->extra_length);
        for (int interfaceIndex = 0; valid && interfaceIndex < configuration->bNumInterfaces;
                ++interfaceIndex) {
            const libusb_interface &interfaceGroup = configuration->interface[interfaceIndex];
            for (int alternateIndex = 0; valid && alternateIndex < interfaceGroup.num_altsetting;
                    ++alternateIndex) {
                const libusb_interface_descriptor &alternate =
                    interfaceGroup.altsetting[alternateIndex];
                raw.insert(raw.end(), {
                    9, LIBUSB_DT_INTERFACE, alternate.bInterfaceNumber,
                    alternate.bAlternateSetting, alternate.bNumEndpoints,
                    alternate.bInterfaceClass, alternate.bInterfaceSubClass,
                    alternate.bInterfaceProtocol, 0});
                valid = appendExtra(raw, alternate.extra, alternate.extra_length);
                for (int endpointIndex = 0; valid && endpointIndex < alternate.bNumEndpoints;
                        ++endpointIndex) {
                    const libusb_endpoint_descriptor &endpoint = alternate.endpoint[endpointIndex];
                    raw.insert(raw.end(), {7, LIBUSB_DT_ENDPOINT, endpoint.bEndpointAddress,
                                           endpoint.bmAttributes});
                    append16(raw, endpoint.wMaxPacketSize);
                    raw.push_back(endpoint.bInterval);
                    valid = appendExtra(raw, endpoint.extra, endpoint.extra_length);
                }
            }
        }
        if (raw.size() > UINT16_MAX) valid = false;
        if (valid) {
            raw[2] = static_cast<std::uint8_t>(raw.size() & 0xff);
            raw[3] = static_cast<std::uint8_t>(raw.size() >> 8);
            candidate.configurationDescriptors.push_back(std::move(raw));
        }
        libusb_free_config_descriptor(configuration);
        if (!valid) {
            diagnostic = "configuration descriptor data exceeded USB limits";
            return BackendStatus::InvalidArgument;
        }
    }
    out = std::move(candidate);
    return BackendStatus::Success;
}

void productionCloseHandle(void *, void *handle) {
    libusb_close(static_cast<libusb_device_handle *>(handle));
}
BackendStatus productionSelectConfiguration(void *, void *handle, std::uint8_t value) {
    return mapLibusb(libusb_set_configuration(
        static_cast<libusb_device_handle *>(handle), value));
}
BackendStatus productionClaimInterface(void *, void *handle, std::uint8_t number) {
    return mapLibusb(libusb_claim_interface(
        static_cast<libusb_device_handle *>(handle), number));
}
BackendStatus productionSelectAlternate(
        void *, void *handle, std::uint8_t number, std::uint8_t alternate) {
    return mapLibusb(libusb_set_interface_alt_setting(
        static_cast<libusb_device_handle *>(handle), number, alternate));
}
BackendStatus productionReleaseInterface(void *, void *handle, std::uint8_t number) {
    return mapLibusb(libusb_release_interface(
        static_cast<libusb_device_handle *>(handle), number));
}
#else
int productionDuplicateFd(void *, int) { return -1; }
void productionCloseFd(void *, int) {}
BackendStatus productionAcquireRuntime(void *, AndroidUsbRuntimeLease &, std::string &) {
    return BackendStatus::UnsupportedOperation;
}
BackendStatus productionWrapDevice(void *, LibusbContext, int, void **, std::string &) {
    return BackendStatus::UnsupportedOperation;
}
BackendStatus productionExtractDescriptors(
        void *, void *, transport::RawDescriptorSet &, std::string &) {
    return BackendStatus::UnsupportedOperation;
}
void productionCloseHandle(void *, void *) {}
BackendStatus unsupportedOperation(void *, void *, std::uint8_t) {
    return BackendStatus::UnsupportedOperation;
}
BackendStatus unsupportedAlternate(void *, void *, std::uint8_t, std::uint8_t) {
    return BackendStatus::UnsupportedOperation;
}
#endif

void cleanupOpenFailure(const AndroidUsbBackendHooks &hooks, int ownedFd,
                        AndroidUsbRuntimeLease &runtime, void *handle) {
    if (handle != nullptr) hooks.closeHandle(hooks.userData, handle);
    if (ownedFd >= 0) hooks.closeFd(hooks.userData, ownedFd);
    runtime.owner.reset();
    runtime.context = nullptr;
}

}  // namespace

bool AndroidUsbBackendHooks::isValid() const noexcept {
    return duplicateFd && closeFd && acquireRuntime && wrapDevice && extractDescriptors &&
        closeHandle && selectConfiguration && claimInterface && selectAlternateSetting &&
        releaseInterface;
}

AndroidUsbBackendFactory::AndroidUsbBackendFactory(AndroidUsbBackendHooks hooks) noexcept
    : hooks_(hooks) {}

BackendStatus AndroidUsbBackendFactory::openAuthorizedFileDescriptor(
        int borrowedFileDescriptor, std::unique_ptr<transport::UsbBackend> &outBackend,
        std::string &outDiagnostic) {
    outBackend.reset();
    outDiagnostic.clear();
    if (!hooks_.isValid() || borrowedFileDescriptor < 0) {
        outDiagnostic = "Android USB backend arguments are invalid";
        return BackendStatus::InvalidArgument;
    }
    const int ownedFd = hooks_.duplicateFd(hooks_.userData, borrowedFileDescriptor);
    if (ownedFd < 0) {
        outDiagnostic = "authorized file descriptor duplication failed";
        return BackendStatus::InternalFailure;
    }
    AndroidUsbRuntimeLease runtime;
    BackendStatus status = hooks_.acquireRuntime(hooks_.userData, runtime, outDiagnostic);
    if (status != BackendStatus::Success || !runtime.owner || runtime.context == nullptr) {
        cleanupOpenFailure(hooks_, ownedFd, runtime, nullptr);
        return status == BackendStatus::Success ? BackendStatus::InternalFailure : status;
    }
    void *handle = nullptr;
    status = hooks_.wrapDevice(
        hooks_.userData, runtime.context, ownedFd, &handle, outDiagnostic);
    if (status != BackendStatus::Success || handle == nullptr) {
        cleanupOpenFailure(hooks_, ownedFd, runtime, handle);
        return status == BackendStatus::Success ? BackendStatus::InternalFailure : status;
    }
    transport::RawDescriptorSet rawDescriptors;
    status = hooks_.extractDescriptors(
        hooks_.userData, handle, rawDescriptors, outDiagnostic);
    transport::DeviceDescriptor descriptor;
    if (status == BackendStatus::Success) {
        std::string parseDiagnostic;
        if (transport::buildDescriptorSnapshot(rawDescriptors, descriptor, parseDiagnostic)
                != USBHOST_OK) {
            outDiagnostic = std::move(parseDiagnostic);
            status = BackendStatus::InvalidArgument;
        }
    }
    if (status != BackendStatus::Success) {
        cleanupOpenFailure(hooks_, ownedFd, runtime, handle);
        return status;
    }
    try {
        outBackend.reset(new AndroidUsbBackend(
            hooks_, ownedFd, std::move(runtime), handle, std::move(descriptor)));
    } catch (const std::bad_alloc &) {
        cleanupOpenFailure(hooks_, ownedFd, runtime, handle);
        outDiagnostic = "Android USB backend allocation failed";
        return BackendStatus::InternalFailure;
    }
    return BackendStatus::Success;
}

AndroidUsbBackend::AndroidUsbBackend(
        AndroidUsbBackendHooks hooks, int ownedFd, AndroidUsbRuntimeLease runtime,
        void *handle, transport::DeviceDescriptor descriptor) noexcept
    : hooks_(hooks), ownedFd_(ownedFd), runtime_(std::move(runtime)), handle_(handle),
      descriptor_(std::move(descriptor)) {}

AndroidUsbBackend::~AndroidUsbBackend() { close(); }

const transport::DeviceDescriptor &AndroidUsbBackend::deviceDescriptor() const noexcept {
    return descriptor_;
}

BackendStatus AndroidUsbBackend::refreshDescriptors() {
    transport::RawDescriptorSet raw;
    std::string diagnostic;
    BackendStatus status = hooks_.extractDescriptors(hooks_.userData, handle_, raw, diagnostic);
    if (status != BackendStatus::Success) return status;
    transport::DeviceDescriptor candidate;
    if (transport::buildDescriptorSnapshot(raw, candidate, diagnostic) != USBHOST_OK) {
        return BackendStatus::InvalidArgument;
    }
    descriptor_ = std::move(candidate);
    return BackendStatus::Success;
}

BackendStatus AndroidUsbBackend::selectConfiguration(std::uint8_t value) {
    if (!handle_) return BackendStatus::InternalFailure;
    const BackendStatus status = hooks_.selectConfiguration(hooks_.userData, handle_, value);
    return status == BackendStatus::Success ? refreshDescriptors() : status;
}
BackendStatus AndroidUsbBackend::claimInterface(std::uint8_t number) {
    return handle_ ? hooks_.claimInterface(hooks_.userData, handle_, number)
                   : BackendStatus::InternalFailure;
}
BackendStatus AndroidUsbBackend::selectAlternateSetting(
        std::uint8_t number, std::uint8_t alternate) {
    if (!handle_) return BackendStatus::InternalFailure;
    const BackendStatus status = hooks_.selectAlternateSetting(
        hooks_.userData, handle_, number, alternate);
    return status == BackendStatus::Success ? refreshDescriptors() : status;
}
BackendStatus AndroidUsbBackend::releaseInterface(std::uint8_t number) {
    return handle_ ? hooks_.releaseInterface(hooks_.userData, handle_, number)
                   : BackendStatus::InternalFailure;
}

transport::OperationId AndroidUsbBackend::submitControl(
        const transport::ControlRequest &, transport::MutableBufferView,
        transport::CompletionCallback completion) {
    if (completion) completion({BackendStatus::UnsupportedOperation, 0, "not implemented"});
    return transport::kInvalidOperationId;
}
transport::OperationId AndroidUsbBackend::submitEndpoint(
        const transport::EndpointTransferRequest &, transport::MutableBufferView,
        transport::CompletionCallback completion) {
    if (completion) completion({BackendStatus::UnsupportedOperation, 0, "not implemented"});
    return transport::kInvalidOperationId;
}
bool AndroidUsbBackend::cancel(transport::OperationId) { return false; }

void AndroidUsbBackend::close() noexcept {
    if (handle_) {
        hooks_.closeHandle(hooks_.userData, handle_);
        handle_ = nullptr;
    }
    if (ownedFd_ >= 0) {
        hooks_.closeFd(hooks_.userData, ownedFd_);
        ownedFd_ = -1;
    }
    runtime_.owner.reset();
    runtime_.context = nullptr;
}

AndroidUsbBackendHooks productionAndroidUsbBackendHooks() noexcept {
#if defined(ANDROID)
    return {nullptr, productionDuplicateFd, productionCloseFd, productionAcquireRuntime,
            productionWrapDevice, productionExtractDescriptors, productionCloseHandle,
            productionSelectConfiguration, productionClaimInterface,
            productionSelectAlternate, productionReleaseInterface};
#else
    return {nullptr, productionDuplicateFd, productionCloseFd, productionAcquireRuntime,
            productionWrapDevice, productionExtractDescriptors, productionCloseHandle,
            unsupportedOperation, unsupportedOperation, unsupportedAlternate,
            unsupportedOperation};
#endif
}

}  // namespace usbhost::android

#ifndef USBHOST_ANDROID_ANDROID_USB_BACKEND_HPP
#define USBHOST_ANDROID_ANDROID_USB_BACKEND_HPP

#include <memory>
#include <string>

#include "android/libusb_runtime.hpp"
#include "transport/descriptors.hpp"
#include "transport/session.hpp"

namespace usbhost::android {

struct AndroidUsbRuntimeLease {
    std::shared_ptr<void> owner;
    LibusbContext context{nullptr};
};

struct AndroidUsbBackendHooks {
    void *userData{nullptr};
    int (*duplicateFd)(void *, int){nullptr};
    void (*closeFd)(void *, int){nullptr};
    transport::BackendStatus (*acquireRuntime)(
        void *, AndroidUsbRuntimeLease &, std::string &){nullptr};
    transport::BackendStatus (*wrapDevice)(
        void *, LibusbContext, int, void **, std::string &){nullptr};
    transport::BackendStatus (*extractDescriptors)(
        void *, void *, transport::RawDescriptorSet &, std::string &){nullptr};
    void (*closeHandle)(void *, void *){nullptr};
    transport::BackendStatus (*selectConfiguration)(void *, void *, std::uint8_t){nullptr};
    transport::BackendStatus (*claimInterface)(void *, void *, std::uint8_t){nullptr};
    transport::BackendStatus (*selectAlternateSetting)(
        void *, void *, std::uint8_t, std::uint8_t){nullptr};
    transport::BackendStatus (*releaseInterface)(void *, void *, std::uint8_t){nullptr};
    transport::OperationId (*submitControlTransfer)(
        void *, void *, const transport::ControlRequest &, transport::MutableBufferView,
        transport::CompletionCallback){nullptr};
    transport::OperationId (*submitEndpointTransfer)(
        void *, void *, const transport::EndpointTransferRequest &,
        transport::MutableBufferView, transport::CompletionCallback){nullptr};
    bool (*cancelTransfer)(void *, transport::OperationId){nullptr};

    bool isValid() const noexcept;
};

AndroidUsbBackendHooks productionAndroidUsbBackendHooks() noexcept;

class AndroidUsbBackend final : public transport::UsbBackend {
public:
    ~AndroidUsbBackend() override;

    const transport::DeviceDescriptor &deviceDescriptor() const noexcept override;
    transport::BackendStatus selectConfiguration(std::uint8_t value) override;
    transport::BackendStatus claimInterface(std::uint8_t interfaceNumber) override;
    transport::BackendStatus selectAlternateSetting(
        std::uint8_t interfaceNumber, std::uint8_t alternateSetting) override;
    transport::BackendStatus releaseInterface(std::uint8_t interfaceNumber) override;
    transport::OperationId submitControl(
        const transport::ControlRequest &, transport::MutableBufferView,
        transport::CompletionCallback) override;
    transport::OperationId submitEndpoint(
        const transport::EndpointTransferRequest &, transport::MutableBufferView,
        transport::CompletionCallback) override;
    bool cancel(transport::OperationId operation) override;
    void close() noexcept override;

private:
    friend class AndroidUsbBackendFactory;
    AndroidUsbBackend(AndroidUsbBackendHooks hooks, int ownedFd,
                      AndroidUsbRuntimeLease runtime, void *handle,
                      transport::DeviceDescriptor descriptor) noexcept;
    transport::BackendStatus refreshDescriptors();

    AndroidUsbBackendHooks hooks_;
    int ownedFd_{-1};
    AndroidUsbRuntimeLease runtime_;
    void *handle_{nullptr};
    transport::DeviceDescriptor descriptor_;
};

class AndroidUsbBackendFactory final : public transport::AuthorizedBackendFactory {
public:
    explicit AndroidUsbBackendFactory(
        AndroidUsbBackendHooks hooks = productionAndroidUsbBackendHooks()) noexcept;

    transport::BackendStatus openAuthorizedFileDescriptor(
        int borrowedFileDescriptor,
        std::unique_ptr<transport::UsbBackend> &outBackend,
        std::string &outDiagnostic) override;

private:
    AndroidUsbBackendHooks hooks_;
};

}  // namespace usbhost::android

#endif

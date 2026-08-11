#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "android/android_usb_backend.hpp"

namespace {

int failures = 0;

#define CHECK_ANDROID_BACKEND(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

struct FakeAndroidUsb {
    std::vector<std::string> events;
    bool duplicateFails{false};
    usbhost::transport::BackendStatus runtimeResult{
        usbhost::transport::BackendStatus::Success};
    usbhost::transport::BackendStatus wrapResult{
        usbhost::transport::BackendStatus::Success};
    usbhost::transport::BackendStatus descriptorResult{
        usbhost::transport::BackendStatus::Success};
    int borrowedFd{-1};
    int closedFd{-1};
};

int duplicateFd(void *userData, int borrowedFd) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    fake.events.push_back("duplicate_fd");
    fake.borrowedFd = borrowedFd;
    return fake.duplicateFails ? -1 : 101;
}

void closeFd(void *userData, int ownedFd) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    fake.events.push_back("close_fd");
    fake.closedFd = ownedFd;
}

usbhost::transport::BackendStatus acquireRuntime(
        void *userData, usbhost::android::AndroidUsbRuntimeLease &outLease,
        std::string &diagnostic) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    fake.events.push_back("acquire_runtime");
    if (fake.runtimeResult != usbhost::transport::BackendStatus::Success) {
        diagnostic = "runtime failed";
        return fake.runtimeResult;
    }
    outLease.context = &fake;
    outLease.owner = std::shared_ptr<void>(&fake, [&fake](void *) {
        fake.events.push_back("release_runtime");
    });
    return usbhost::transport::BackendStatus::Success;
}

usbhost::transport::BackendStatus wrapDevice(
        void *userData, usbhost::android::LibusbContext context, int ownedFd,
        void **outHandle, std::string &diagnostic) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    fake.events.push_back("wrap_device");
    CHECK_ANDROID_BACKEND(context == &fake);
    CHECK_ANDROID_BACKEND(ownedFd == 101);
    if (fake.wrapResult != usbhost::transport::BackendStatus::Success) {
        diagnostic = "wrap failed";
        return fake.wrapResult;
    }
    *outHandle = &fake;
    return usbhost::transport::BackendStatus::Success;
}

usbhost::transport::BackendStatus extractDescriptors(
        void *userData, void *handle, usbhost::transport::RawDescriptorSet &outDescriptors,
        std::string &diagnostic) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    fake.events.push_back("extract_descriptors");
    CHECK_ANDROID_BACKEND(handle == &fake);
    if (fake.descriptorResult != usbhost::transport::BackendStatus::Success) {
        diagnostic = "descriptor failed";
        return fake.descriptorResult;
    }
    outDescriptors.deviceDescriptor = {
        18, 1, 0x10, 0x02, 0, 0, 0, 64,
        0x34, 0x12, 0x78, 0x56, 0, 1, 0, 0, 0, 1};
    outDescriptors.configurationDescriptors = {{9, 2, 9, 0, 0, 1, 0, 0x80, 50}};
    outDescriptors.activeConfigurationValue = 1;
    outDescriptors.generation = usbhost::transport::SnapshotGeneration::initial();
    return usbhost::transport::BackendStatus::Success;
}

void closeHandle(void *userData, void *handle) {
    auto &fake = *static_cast<FakeAndroidUsb *>(userData);
    CHECK_ANDROID_BACKEND(handle == &fake);
    fake.events.push_back("close_handle");
}

usbhost::transport::BackendStatus successfulOperation(void *, void *, std::uint8_t) {
    return usbhost::transport::BackendStatus::Success;
}

usbhost::transport::BackendStatus successfulAlternate(
        void *, void *, std::uint8_t, std::uint8_t) {
    return usbhost::transport::BackendStatus::Success;
}

usbhost::android::AndroidUsbBackendHooks hooks(FakeAndroidUsb &fake) {
    return {&fake, duplicateFd, closeFd, acquireRuntime, wrapDevice, extractDescriptors,
            closeHandle, successfulOperation, successfulOperation,
            successfulAlternate, successfulOperation};
}

void successfulOwnershipAndCleanupTest() {
    using namespace usbhost::transport;
    FakeAndroidUsb fake;
    usbhost::android::AndroidUsbBackendFactory factory(hooks(fake));
    std::unique_ptr<UsbBackend> backend;
    std::string diagnostic;
    CHECK_ANDROID_BACKEND(factory.openAuthorizedFileDescriptor(77, backend, diagnostic)
                          == BackendStatus::Success);
    CHECK_ANDROID_BACKEND(backend != nullptr);
    CHECK_ANDROID_BACKEND(diagnostic.empty());
    CHECK_ANDROID_BACKEND(fake.borrowedFd == 77);
    CHECK_ANDROID_BACKEND(fake.closedFd == -1);
    CHECK_ANDROID_BACKEND(backend->deviceDescriptor().vendorId == 0x1234);
    backend->close();
    backend->close();
    backend.reset();
    CHECK_ANDROID_BACKEND(fake.closedFd == 101);
    CHECK_ANDROID_BACKEND(fake.events == std::vector<std::string>({
        "duplicate_fd", "acquire_runtime", "wrap_device", "extract_descriptors",
        "close_handle", "close_fd", "release_runtime"}));
}

void failureCleanupTest() {
    using namespace usbhost::transport;
    std::unique_ptr<UsbBackend> backend;
    std::string diagnostic;

    FakeAndroidUsb duplicateFailure;
    duplicateFailure.duplicateFails = true;
    usbhost::android::AndroidUsbBackendFactory duplicateFactory(hooks(duplicateFailure));
    CHECK_ANDROID_BACKEND(duplicateFactory.openAuthorizedFileDescriptor(3, backend, diagnostic)
                          == BackendStatus::InternalFailure);
    CHECK_ANDROID_BACKEND(duplicateFailure.events
                          == std::vector<std::string>({"duplicate_fd"}));

    FakeAndroidUsb runtimeFailure;
    runtimeFailure.runtimeResult = BackendStatus::InternalFailure;
    usbhost::android::AndroidUsbBackendFactory runtimeFactory(hooks(runtimeFailure));
    CHECK_ANDROID_BACKEND(runtimeFactory.openAuthorizedFileDescriptor(4, backend, diagnostic)
                          == BackendStatus::InternalFailure);
    CHECK_ANDROID_BACKEND(runtimeFailure.events == std::vector<std::string>({
        "duplicate_fd", "acquire_runtime", "close_fd"}));

    FakeAndroidUsb wrapFailure;
    wrapFailure.wrapResult = BackendStatus::Disconnected;
    usbhost::android::AndroidUsbBackendFactory wrapFactory(hooks(wrapFailure));
    CHECK_ANDROID_BACKEND(wrapFactory.openAuthorizedFileDescriptor(5, backend, diagnostic)
                          == BackendStatus::Disconnected);
    CHECK_ANDROID_BACKEND(wrapFailure.events == std::vector<std::string>({
        "duplicate_fd", "acquire_runtime", "wrap_device", "close_fd", "release_runtime"}));

    FakeAndroidUsb descriptorFailure;
    descriptorFailure.descriptorResult = BackendStatus::UsbFailure;
    usbhost::android::AndroidUsbBackendFactory descriptorFactory(hooks(descriptorFailure));
    CHECK_ANDROID_BACKEND(descriptorFactory.openAuthorizedFileDescriptor(6, backend, diagnostic)
                          == BackendStatus::UsbFailure);
    CHECK_ANDROID_BACKEND(descriptorFailure.events == std::vector<std::string>({
        "duplicate_fd", "acquire_runtime", "wrap_device", "extract_descriptors",
        "close_handle", "close_fd", "release_runtime"}));
}

}  // namespace

int runAndroidUsbBackendContractTest() {
    failures = 0;
    successfulOwnershipAndCleanupTest();
    failureCleanupTest();
    return failures;
}

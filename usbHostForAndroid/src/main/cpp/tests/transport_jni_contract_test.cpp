#include "jni/transport_jni_contract.hpp"

#include <cstdint>
#include <type_traits>

#include "usbhost/transport.h"

using usbhost::jni::DescriptorLocation;
using usbhost::jni::NativeCallResult;

extern "C" void usbhost_test_transport_install_fixture(int open_status);

static_assert(std::is_trivially_copyable_v<NativeCallResult>);
static_assert(std::is_trivially_copyable_v<DescriptorLocation>);

int runTransportJniContractTest() {
    const NativeCallResult success = usbhost::jni::callResult(USBHOST_OK, 42);
    const NativeCallResult failure = usbhost::jni::callResult(USBHOST_INVALID_STATE, 99);
    const DescriptorLocation location = usbhost::jni::descriptorLocation(
        USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT, 7, 1, 2, 3, 4, 5);

    usbhost_test_transport_install_fixture(USBHOST_OK);
    usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
    const bool opened = usbhost_transport_open_fd(73, &session) == USBHOST_OK;
    const bool cancelWithoutTransfer = usbhost_transport_cancel(session) == USBHOST_INVALID_STATE;
    const bool closed = usbhost_transport_close(session) == USBHOST_OK;

    return opened && cancelWithoutTransfer && closed
            && usbhost::jni::kOpenRecordLength == 2
            && usbhost::jni::kDeviceRecordLength == 11
            && usbhost::jni::kConfigurationRecordLength == 9
            && usbhost::jni::kInterfaceRecordLength == 7
            && usbhost::jni::kAlternateRecordLength == 10
            && usbhost::jni::kEndpointRecordLength == 10
            && success.status == USBHOST_OK
            && success.value == 42
            && failure.status == USBHOST_INVALID_STATE
            && failure.value == 0
            && location.scope == USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT
            && location.snapshotGeneration == 7
            && location.configurationIndex == 1
            && location.interfaceIndex == 2
            && location.alternateSettingIndex == 3
            && location.endpointIndex == 4
            && location.additionalDescriptorIndex == 5
        ? 0
        : 1;
}

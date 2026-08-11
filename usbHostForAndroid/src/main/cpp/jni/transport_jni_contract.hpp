#ifndef USBHOST_TRANSPORT_JNI_CONTRACT_HPP
#define USBHOST_TRANSPORT_JNI_CONTRACT_HPP

#include <cstdint>

#include "usbhost/transport.h"

namespace usbhost::jni {

struct NativeCallResult {
    std::int32_t status;
    std::uint64_t value;
};

constexpr NativeCallResult callResult(usbhost_status status, std::uint64_t value) noexcept {
    return {static_cast<std::int32_t>(status), status == USBHOST_OK ? value : 0};
}

struct DescriptorLocation {
    std::uint32_t scope;
    std::uint64_t snapshotGeneration;
    std::uint32_t configurationIndex;
    std::uint32_t interfaceIndex;
    std::uint32_t alternateSettingIndex;
    std::uint32_t endpointIndex;
    std::uint32_t additionalDescriptorIndex;
};

constexpr DescriptorLocation descriptorLocation(
        std::uint32_t scope, std::uint64_t generation,
        std::uint32_t configurationIndex, std::uint32_t interfaceIndex,
        std::uint32_t alternateSettingIndex, std::uint32_t endpointIndex,
        std::uint32_t additionalDescriptorIndex) noexcept {
    return {scope, generation, configurationIndex, interfaceIndex,
            alternateSettingIndex, endpointIndex, additionalDescriptorIndex};
}

constexpr std::uint32_t kOpenRecordLength = 2;
constexpr std::uint32_t kDeviceRecordLength = 11;
constexpr std::uint32_t kConfigurationRecordLength = 9;
constexpr std::uint32_t kInterfaceRecordLength = 7;
constexpr std::uint32_t kAlternateRecordLength = 10;
constexpr std::uint32_t kEndpointRecordLength = 10;

}  // namespace usbhost::jni

#endif

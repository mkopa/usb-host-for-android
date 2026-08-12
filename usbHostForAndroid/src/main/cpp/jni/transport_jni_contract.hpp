#ifndef USBHOST_TRANSPORT_JNI_CONTRACT_HPP
#define USBHOST_TRANSPORT_JNI_CONTRACT_HPP

#include <cstdint>

#include "usbhost/transport.h"

namespace usbhost::jni {

struct NativeCallResult {
    std::int32_t status;
    std::uint64_t value;
};

struct TransferSlice {
    std::uint32_t offset;
    std::uint32_t length;
};

constexpr bool checkedTransferSlice(
        std::int32_t capacity, std::int32_t offset, std::int32_t length,
        TransferSlice &out) noexcept {
    if (capacity < 0 || offset < 0 || length < 0 || offset > capacity ||
        length > capacity - offset) return false;
    out = {static_cast<std::uint32_t>(offset), static_cast<std::uint32_t>(length)};
    return true;
}

struct CompletedInputCopy {
    std::uint32_t destinationOffset;
    std::uint32_t length;
};

constexpr bool completedInputCopy(
        TransferSlice requested, std::uint32_t actualLength,
        CompletedInputCopy &out) noexcept {
    if (actualLength > requested.length) return false;
    out = {requested.offset, actualLength};
    return true;
}

struct NativeTransferResult {
    usbhost_status status;
    std::uint32_t actualLength;
};

template <typename Operation>
NativeTransferResult executeTransferNoexcept(Operation &&operation) noexcept {
    try {
        return operation();
    } catch (...) {
        return {USBHOST_INTERNAL_ERROR, 0};
    }
}

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
constexpr std::uint32_t kTransferRecordLength = 2;

}  // namespace usbhost::jni

#endif

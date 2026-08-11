#ifndef USBHOST_TRANSPORT_TYPES_HPP
#define USBHOST_TRANSPORT_TYPES_HPP

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "usbhost/transport.h"

namespace usbhost::transport {

enum class Direction : std::uint8_t {
    Out = USBHOST_TRANSPORT_DIRECTION_OUT,
    In = USBHOST_TRANSPORT_DIRECTION_IN
};

enum class TransferType : std::uint8_t {
    Control = USBHOST_TRANSPORT_TRANSFER_CONTROL,
    Isochronous = USBHOST_TRANSPORT_TRANSFER_ISOCHRONOUS,
    Bulk = USBHOST_TRANSPORT_TRANSFER_BULK,
    Interrupt = USBHOST_TRANSPORT_TRANSFER_INTERRUPT
};

enum class DescriptorScope : std::uint8_t {
    Configuration = USBHOST_TRANSPORT_DESCRIPTOR_CONFIGURATION,
    AlternateSetting = USBHOST_TRANSPORT_DESCRIPTOR_ALTERNATE_SETTING,
    Endpoint = USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT
};

enum class SessionState : std::uint8_t {
    Opening,
    Open,
    Closing,
    Closed,
    Failed
};

class SnapshotGeneration final {
public:
    constexpr SnapshotGeneration() noexcept = default;

    static constexpr SnapshotGeneration initial() noexcept {
        return SnapshotGeneration(UINT64_C(1));
    }

    static constexpr SnapshotGeneration fromRaw(std::uint64_t value) noexcept {
        return SnapshotGeneration(value);
    }

    constexpr bool isValid() const noexcept {
        return value_ != 0;
    }

    constexpr std::uint64_t value() const noexcept {
        return value_;
    }

    constexpr SnapshotGeneration next() const noexcept {
        return !isValid() || value_ == std::numeric_limits<std::uint64_t>::max()
            ? initial()
            : SnapshotGeneration(value_ + UINT64_C(1));
    }

    friend constexpr bool operator==(SnapshotGeneration left,
                                     SnapshotGeneration right) noexcept {
        return left.value_ == right.value_;
    }

    friend constexpr bool operator!=(SnapshotGeneration left,
                                     SnapshotGeneration right) noexcept {
        return !(left == right);
    }

private:
    constexpr explicit SnapshotGeneration(std::uint64_t value) noexcept : value_(value) {}

    std::uint64_t value_{0};
};

struct BufferSlice {
    std::uint32_t offset{0};
    std::uint32_t length{0};

    constexpr bool fits(std::uint32_t capacity) const noexcept {
        return offset <= capacity && length <= capacity - offset;
    }
};

struct AdditionalDescriptor {
    std::uint8_t type{0};
    std::vector<std::uint8_t> bytes;
};

struct EndpointDescriptor {
    std::uint8_t address{0};
    std::uint8_t number{0};
    Direction direction{Direction::Out};
    TransferType transferType{TransferType::Control};
    std::uint16_t maximumPacketSize{0};
    std::uint8_t interval{0};
    std::uint8_t interfaceNumber{0};
    std::uint8_t alternateSetting{0};
    SnapshotGeneration generation;
    std::vector<AdditionalDescriptor> additionalDescriptors;
};

struct AlternateSettingDescriptor {
    std::uint8_t interfaceNumber{0};
    std::uint8_t alternateSetting{0};
    std::uint8_t interfaceClass{0};
    std::uint8_t interfaceSubclass{0};
    std::uint8_t interfaceProtocol{0};
    SnapshotGeneration generation;
    std::vector<EndpointDescriptor> endpoints;
    std::vector<AdditionalDescriptor> additionalDescriptors;
};

struct InterfaceDescriptor {
    std::uint8_t interfaceNumber{0};
    std::uint8_t activeAlternateSetting{0};
    bool claimed{false};
    SnapshotGeneration generation;
    std::vector<AlternateSettingDescriptor> alternateSettings;
};

struct ConfigurationDescriptor {
    std::uint8_t configurationValue{0};
    std::uint8_t attributes{0};
    std::uint8_t maximumPower{0};
    bool active{false};
    SnapshotGeneration generation;
    std::vector<InterfaceDescriptor> interfaces;
    std::vector<AdditionalDescriptor> additionalDescriptors;
};

struct DeviceDescriptor {
    std::uint16_t usbVersionBcd{0};
    std::uint8_t deviceClass{0};
    std::uint8_t deviceSubclass{0};
    std::uint8_t deviceProtocol{0};
    std::uint8_t endpointZeroMaximumPacketSize{0};
    std::uint16_t vendorId{0};
    std::uint16_t productId{0};
    std::uint16_t deviceReleaseBcd{0};
    SnapshotGeneration generation;
    std::vector<ConfigurationDescriptor> configurations;
};

struct ControlRequest {
    std::uint8_t requestType{0};
    std::uint8_t request{0};
    std::uint16_t value{0};
    std::uint16_t index{0};
    Direction direction{Direction::Out};
    BufferSlice buffer;
    std::uint32_t timeoutMilliseconds{0};
    SnapshotGeneration generation;
};

struct EndpointTransferRequest {
    TransferType transferType{TransferType::Bulk};
    std::uint8_t endpointAddress{0};
    Direction direction{Direction::Out};
    BufferSlice buffer;
    std::uint32_t timeoutMilliseconds{0};
    SnapshotGeneration generation;
};

struct TransferResult {
    usbhost_status status{USBHOST_OK};
    std::uint32_t actualLength{0};
    std::string diagnostic;

    bool isOk() const noexcept {
        return status == USBHOST_OK;
    }
};

}  // namespace usbhost::transport

#endif

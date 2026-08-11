#include "transport/descriptors.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace usbhost::transport {

namespace {

constexpr std::uint8_t kDeviceDescriptorType = 1;
constexpr std::uint8_t kConfigurationDescriptorType = 2;
constexpr std::uint8_t kInterfaceDescriptorType = 4;
constexpr std::uint8_t kEndpointDescriptorType = 5;
constexpr std::size_t kDeviceDescriptorLength = 18;
constexpr std::size_t kConfigurationDescriptorLength = 9;
constexpr std::size_t kInterfaceDescriptorLength = 9;
constexpr std::size_t kEndpointDescriptorLength = 7;
constexpr std::size_t kMaximumRawConfigurationLength = 65535;

std::uint16_t readLittleEndian16(const std::vector<std::uint8_t> &bytes,
                                 std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

usbhost_status fail(std::string &diagnostic, const char *message) {
    diagnostic = message;
    return USBHOST_INVALID_ARGUMENT;
}

TransferType transferType(std::uint8_t attributes) noexcept {
    switch (attributes & 0x03) {
        case 0: return TransferType::Control;
        case 1: return TransferType::Isochronous;
        case 2: return TransferType::Bulk;
        default: return TransferType::Interrupt;
    }
}

bool finishAlternate(const AlternateSettingDescriptor *alternate,
                     std::uint8_t expectedEndpoints,
                     std::string &diagnostic) {
    if (alternate != nullptr && alternate->endpoints.size() != expectedEndpoints) {
        diagnostic = "interface endpoint count does not match descriptors";
        return false;
    }
    return true;
}

usbhost_status parseConfiguration(const std::vector<std::uint8_t> &raw,
                                  std::uint8_t activeValue,
                                  SnapshotGeneration generation,
                                  ConfigurationDescriptor &output,
                                  std::string &diagnostic) {
    if (raw.size() < kConfigurationDescriptorLength ||
        raw.size() > kMaximumRawConfigurationLength) {
        return fail(diagnostic, "configuration descriptor length is out of range");
    }
    if (raw[0] < kConfigurationDescriptorLength || raw[1] != kConfigurationDescriptorType) {
        return fail(diagnostic, "configuration descriptor header is invalid");
    }
    const std::uint16_t totalLength = readLittleEndian16(raw, 2);
    if (totalLength != raw.size() || totalLength < raw[0]) {
        return fail(diagnostic, "configuration total length does not match input");
    }

    ConfigurationDescriptor candidate;
    candidate.configurationValue = raw[5];
    candidate.attributes = raw[7];
    candidate.maximumPower = raw[8];
    candidate.active = raw[5] == activeValue;
    candidate.generation = generation;
    const std::uint8_t expectedInterfaces = raw[4];

    AlternateSettingDescriptor *currentAlternate = nullptr;
    std::uint8_t expectedEndpoints = 0;
    std::size_t offset = raw[0];
    while (offset < raw.size()) {
        if (raw.size() - offset < 2) {
            return fail(diagnostic, "descriptor header is truncated");
        }
        const std::size_t length = raw[offset];
        const std::uint8_t type = raw[offset + 1];
        if (length < 2 || length > raw.size() - offset) {
            return fail(diagnostic, "descriptor length is invalid");
        }

        if (type == kConfigurationDescriptorType) {
            return fail(diagnostic, "duplicate configuration header");
        }
        if (type == kInterfaceDescriptorType) {
            if (length < kInterfaceDescriptorLength) {
                return fail(diagnostic, "interface descriptor is truncated");
            }
            if (!finishAlternate(currentAlternate, expectedEndpoints, diagnostic)) {
                return USBHOST_INVALID_ARGUMENT;
            }
            const std::uint8_t interfaceNumber = raw[offset + 2];
            const std::uint8_t alternateSetting = raw[offset + 3];
            auto interfaceIterator = std::find_if(
                candidate.interfaces.begin(), candidate.interfaces.end(),
                [interfaceNumber](const InterfaceDescriptor &descriptor) {
                    return descriptor.interfaceNumber == interfaceNumber;
                });
            if (interfaceIterator == candidate.interfaces.end()) {
                InterfaceDescriptor descriptor;
                descriptor.interfaceNumber = interfaceNumber;
                descriptor.activeAlternateSetting = alternateSetting;
                descriptor.generation = generation;
                candidate.interfaces.push_back(std::move(descriptor));
                interfaceIterator = std::prev(candidate.interfaces.end());
            }
            const bool duplicateAlternate = std::any_of(
                interfaceIterator->alternateSettings.begin(),
                interfaceIterator->alternateSettings.end(),
                [alternateSetting](const AlternateSettingDescriptor &descriptor) {
                    return descriptor.alternateSetting == alternateSetting;
                });
            if (duplicateAlternate) {
                return fail(diagnostic, "duplicate interface alternate setting");
            }
            AlternateSettingDescriptor alternate;
            alternate.interfaceNumber = interfaceNumber;
            alternate.alternateSetting = alternateSetting;
            alternate.interfaceClass = raw[offset + 5];
            alternate.interfaceSubclass = raw[offset + 6];
            alternate.interfaceProtocol = raw[offset + 7];
            alternate.generation = generation;
            interfaceIterator->alternateSettings.push_back(std::move(alternate));
            currentAlternate = &interfaceIterator->alternateSettings.back();
            expectedEndpoints = raw[offset + 4];
        } else if (type == kEndpointDescriptorType) {
            if (length < kEndpointDescriptorLength || currentAlternate == nullptr) {
                return fail(diagnostic, "endpoint descriptor has no valid interface parent");
            }
            const std::uint8_t address = raw[offset + 2];
            if ((address & 0x0f) == 0) {
                return fail(diagnostic, "endpoint zero cannot appear in an interface");
            }
            const bool duplicateEndpoint = std::any_of(
                currentAlternate->endpoints.begin(), currentAlternate->endpoints.end(),
                [address](const EndpointDescriptor &descriptor) {
                    return descriptor.address == address;
                });
            if (duplicateEndpoint) {
                return fail(diagnostic, "duplicate endpoint address");
            }
            EndpointDescriptor endpoint;
            endpoint.address = address;
            endpoint.number = address & 0x0f;
            endpoint.direction = (address & 0x80) == 0 ? Direction::Out : Direction::In;
            endpoint.transferType = transferType(raw[offset + 3]);
            endpoint.maximumPacketSize = readLittleEndian16(raw, offset + 4);
            endpoint.interval = raw[offset + 6];
            endpoint.interfaceNumber = currentAlternate->interfaceNumber;
            endpoint.alternateSetting = currentAlternate->alternateSetting;
            endpoint.generation = generation;
            currentAlternate->endpoints.push_back(std::move(endpoint));
        }
        offset += length;
    }

    if (!finishAlternate(currentAlternate, expectedEndpoints, diagnostic)) {
        return USBHOST_INVALID_ARGUMENT;
    }
    if (candidate.interfaces.size() != expectedInterfaces) {
        return fail(diagnostic, "configuration interface count does not match descriptors");
    }
    output = std::move(candidate);
    return USBHOST_OK;
}

}  // namespace

usbhost_status buildDescriptorSnapshot(const RawDescriptorSet &input,
                                       DeviceDescriptor &outDescriptor,
                                       std::string &outDiagnostic) {
    std::string diagnostic;
    if (!input.generation.isValid()) {
        return fail(outDiagnostic, "snapshot generation is invalid");
    }
    const auto &rawDevice = input.deviceDescriptor;
    if (rawDevice.size() != kDeviceDescriptorLength ||
        rawDevice[0] != kDeviceDescriptorLength || rawDevice[1] != kDeviceDescriptorType) {
        return fail(outDiagnostic, "device descriptor is invalid");
    }
    if (rawDevice[17] != input.configurationDescriptors.size()) {
        return fail(outDiagnostic, "device configuration count does not match input");
    }

    DeviceDescriptor candidate;
    candidate.usbVersionBcd = readLittleEndian16(rawDevice, 2);
    candidate.deviceClass = rawDevice[4];
    candidate.deviceSubclass = rawDevice[5];
    candidate.deviceProtocol = rawDevice[6];
    candidate.endpointZeroMaximumPacketSize = rawDevice[7];
    candidate.vendorId = readLittleEndian16(rawDevice, 8);
    candidate.productId = readLittleEndian16(rawDevice, 10);
    candidate.deviceReleaseBcd = readLittleEndian16(rawDevice, 12);
    candidate.generation = input.generation;

    std::array<bool, 256> configurationValues{};
    bool activeFound = input.activeConfigurationValue == 0;
    for (const auto &rawConfiguration : input.configurationDescriptors) {
        ConfigurationDescriptor configuration;
        const usbhost_status status = parseConfiguration(
            rawConfiguration, input.activeConfigurationValue, input.generation,
            configuration, diagnostic);
        if (status != USBHOST_OK) {
            outDiagnostic = std::move(diagnostic);
            return status;
        }
        if (configuration.configurationValue == 0 ||
            configurationValues[configuration.configurationValue]) {
            return fail(outDiagnostic, "configuration value is zero or duplicated");
        }
        configurationValues[configuration.configurationValue] = true;
        activeFound = activeFound || configuration.active;
        candidate.configurations.push_back(std::move(configuration));
    }
    if (!activeFound) {
        return fail(outDiagnostic, "active configuration is absent");
    }

    outDescriptor = std::move(candidate);
    outDiagnostic.clear();
    return USBHOST_OK;
}

}  // namespace usbhost::transport

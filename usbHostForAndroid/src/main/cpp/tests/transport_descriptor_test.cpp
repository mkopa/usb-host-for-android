#include <cstdint>
#include <string>
#include <vector>

#include "transport/descriptors.hpp"

namespace {

int failures = 0;

#define CHECK_DESCRIPTOR(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

std::vector<std::uint8_t> deviceDescriptor(std::uint8_t configurationCount = 1) {
    return {18, 1, 0x10, 0x02, 0xef, 0x02, 0x01, 64,
            0x83, 0x04, 0x40, 0x57, 0x34, 0x12, 0, 0, 0, configurationCount};
}

std::vector<std::uint8_t> configurationDescriptor(std::uint8_t value = 1) {
    return {9, 2, 25, 0, 1, value, 0, 0x80, 50,
            9, 4, 0, 0, 1, 0xff, 0x01, 0x02, 0,
            7, 5, 0x81, 0x02, 64, 0, 1};
}

usbhost::transport::RawDescriptorSet validSet() {
    usbhost::transport::RawDescriptorSet input;
    input.deviceDescriptor = deviceDescriptor();
    input.configurationDescriptors = {configurationDescriptor()};
    input.activeConfigurationValue = 1;
    input.generation = usbhost::transport::SnapshotGeneration::initial();
    return input;
}

void validSnapshotTest() {
    using namespace usbhost::transport;
    DeviceDescriptor output;
    std::string diagnostic;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(validSet(), output, diagnostic) == USBHOST_OK);
    CHECK_DESCRIPTOR(diagnostic.empty());
    CHECK_DESCRIPTOR(output.usbVersionBcd == 0x0210);
    CHECK_DESCRIPTOR(output.deviceClass == 0xef);
    CHECK_DESCRIPTOR(output.vendorId == 0x0483);
    CHECK_DESCRIPTOR(output.productId == 0x5740);
    CHECK_DESCRIPTOR(output.deviceReleaseBcd == 0x1234);
    CHECK_DESCRIPTOR(output.generation == SnapshotGeneration::initial());
    CHECK_DESCRIPTOR(output.configurations.size() == 1);
    const auto &configuration = output.configurations[0];
    CHECK_DESCRIPTOR(configuration.configurationValue == 1);
    CHECK_DESCRIPTOR(configuration.active);
    CHECK_DESCRIPTOR(configuration.interfaces.size() == 1);
    const auto &interfaceDescriptor = configuration.interfaces[0];
    CHECK_DESCRIPTOR(interfaceDescriptor.interfaceNumber == 0);
    CHECK_DESCRIPTOR(interfaceDescriptor.activeAlternateSetting == 0);
    CHECK_DESCRIPTOR(interfaceDescriptor.alternateSettings.size() == 1);
    const auto &alternate = interfaceDescriptor.alternateSettings[0];
    CHECK_DESCRIPTOR(alternate.interfaceClass == 0xff);
    CHECK_DESCRIPTOR(alternate.interfaceSubclass == 1);
    CHECK_DESCRIPTOR(alternate.interfaceProtocol == 2);
    CHECK_DESCRIPTOR(alternate.endpoints.size() == 1);
    const auto &endpoint = alternate.endpoints[0];
    CHECK_DESCRIPTOR(endpoint.address == 0x81);
    CHECK_DESCRIPTOR(endpoint.number == 1);
    CHECK_DESCRIPTOR(endpoint.direction == Direction::In);
    CHECK_DESCRIPTOR(endpoint.transferType == TransferType::Bulk);
    CHECK_DESCRIPTOR(endpoint.maximumPacketSize == 64);
    CHECK_DESCRIPTOR(endpoint.interfaceNumber == 0);
    CHECK_DESCRIPTOR(endpoint.alternateSetting == 0);
}

void malformedAndAtomicFailureTest() {
    using namespace usbhost::transport;
    DeviceDescriptor output;
    output.vendorId = 0xbeef;
    std::string diagnostic;

    auto malformed = validSet();
    malformed.configurationDescriptors[0][2] = 26;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(malformed, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);
    CHECK_DESCRIPTOR(output.vendorId == 0xbeef);
    CHECK_DESCRIPTOR(!diagnostic.empty());

    malformed = validSet();
    malformed.configurationDescriptors[0].insert(
        malformed.configurationDescriptors[0].end(), {0, 0x30});
    malformed.configurationDescriptors[0][2] = 27;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(malformed, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);
    CHECK_DESCRIPTOR(output.vendorId == 0xbeef);

    malformed = validSet();
    malformed.configurationDescriptors[0].resize(65536, 0);
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(malformed, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);
    CHECK_DESCRIPTOR(output.vendorId == 0xbeef);
}

void duplicateAndCountFailureTest() {
    using namespace usbhost::transport;
    DeviceDescriptor output;
    std::string diagnostic;

    auto duplicateConfiguration = validSet();
    duplicateConfiguration.deviceDescriptor = deviceDescriptor(2);
    duplicateConfiguration.configurationDescriptors.push_back(configurationDescriptor());
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(duplicateConfiguration, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);

    auto duplicateAlternate = validSet();
    auto &alternateBytes = duplicateAlternate.configurationDescriptors[0];
    alternateBytes.insert(alternateBytes.end(), {9, 4, 0, 0, 0, 0xff, 0, 0, 0});
    alternateBytes[2] = 34;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(duplicateAlternate, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);

    auto duplicateEndpoint = validSet();
    auto &endpointBytes = duplicateEndpoint.configurationDescriptors[0];
    endpointBytes[13] = 2;
    endpointBytes.insert(endpointBytes.end(), {7, 5, 0x81, 0x02, 64, 0, 1});
    endpointBytes[2] = 32;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(duplicateEndpoint, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);

    auto wrongInterfaceCount = validSet();
    wrongInterfaceCount.configurationDescriptors[0][4] = 2;
    CHECK_DESCRIPTOR(buildDescriptorSnapshot(wrongInterfaceCount, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);
}

}  // namespace

int runTransportDescriptorTest() {
    failures = 0;
    validSnapshotTest();
    malformedAndAtomicFailureTest();
    duplicateAndCountFailureTest();
    return failures;
}

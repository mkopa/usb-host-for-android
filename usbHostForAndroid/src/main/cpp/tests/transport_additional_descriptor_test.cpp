#include <cstdint>
#include <string>
#include <vector>

#include "transport/descriptors.hpp"

namespace {

int failures = 0;

#define CHECK_ADDITIONAL(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

usbhost::transport::RawDescriptorSet descriptorSetWithExtras() {
    usbhost::transport::RawDescriptorSet input;
    input.deviceDescriptor = {18, 1, 0x10, 0x02, 0, 0, 0, 64,
                              0x34, 0x12, 0x78, 0x56, 0, 1, 0, 0, 0, 1};
    input.configurationDescriptors = {{
        9, 2, 37, 0, 1, 1, 0, 0x80, 50,
        4, 0x30, 0xaa, 0xbb,
        9, 4, 0, 0, 1, 0xff, 0, 0, 0,
        3, 0x24, 0xcc,
        7, 5, 0x81, 0x03, 16, 0, 4,
        5, 0x25, 0xdd, 0xee, 0xff}};
    input.activeConfigurationValue = 1;
    input.generation = usbhost::transport::SnapshotGeneration::initial();
    return input;
}

void scopedOwnedRecordsTest() {
    using namespace usbhost::transport;
    RawDescriptorSet input = descriptorSetWithExtras();
    DeviceDescriptor output;
    std::string diagnostic;
    CHECK_ADDITIONAL(buildDescriptorSnapshot(input, output, diagnostic) == USBHOST_OK);
    CHECK_ADDITIONAL(diagnostic.empty());

    const auto &configuration = output.configurations[0];
    const auto &alternate = configuration.interfaces[0].alternateSettings[0];
    const auto &endpoint = alternate.endpoints[0];
    CHECK_ADDITIONAL(configuration.additionalDescriptors.size() == 1);
    CHECK_ADDITIONAL(configuration.additionalDescriptors[0].type == 0x30);
    CHECK_ADDITIONAL(configuration.additionalDescriptors[0].bytes
                     == std::vector<std::uint8_t>({4, 0x30, 0xaa, 0xbb}));
    CHECK_ADDITIONAL(alternate.additionalDescriptors.size() == 1);
    CHECK_ADDITIONAL(alternate.additionalDescriptors[0].type == 0x24);
    CHECK_ADDITIONAL(alternate.additionalDescriptors[0].bytes
                     == std::vector<std::uint8_t>({3, 0x24, 0xcc}));
    CHECK_ADDITIONAL(endpoint.additionalDescriptors.size() == 1);
    CHECK_ADDITIONAL(endpoint.additionalDescriptors[0].type == 0x25);
    CHECK_ADDITIONAL(endpoint.additionalDescriptors[0].bytes
                     == std::vector<std::uint8_t>({5, 0x25, 0xdd, 0xee, 0xff}));

    input.configurationDescriptors[0][11] = 0;
    CHECK_ADDITIONAL(configuration.additionalDescriptors[0].bytes[2] == 0xaa);
}

void malformedAdditionalDescriptorIsAtomicTest() {
    using namespace usbhost::transport;
    RawDescriptorSet input = descriptorSetWithExtras();
    input.configurationDescriptors[0][9] = 1;
    DeviceDescriptor output;
    output.productId = 0xbeef;
    std::string diagnostic;
    CHECK_ADDITIONAL(buildDescriptorSnapshot(input, output, diagnostic)
                     == USBHOST_INVALID_ARGUMENT);
    CHECK_ADDITIONAL(output.productId == 0xbeef);
    CHECK_ADDITIONAL(!diagnostic.empty());
}

}  // namespace

int runTransportAdditionalDescriptorTest() {
    failures = 0;
    scopedOwnedRecordsTest();
    malformedAdditionalDescriptorIsAtomicTest();
    return failures;
}

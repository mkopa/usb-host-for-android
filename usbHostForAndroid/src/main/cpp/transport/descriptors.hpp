#ifndef USBHOST_TRANSPORT_DESCRIPTORS_HPP
#define USBHOST_TRANSPORT_DESCRIPTORS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "transport/types.hpp"

namespace usbhost::transport {

/** Owned raw descriptors supplied by an authorized backend; no native USB pointer is retained. */
struct RawDescriptorSet {
    std::vector<std::uint8_t> deviceDescriptor;
    std::vector<std::vector<std::uint8_t>> configurationDescriptors;
    std::uint8_t activeConfigurationValue{0};
    SnapshotGeneration generation{SnapshotGeneration::initial()};
};

/** Builds the full snapshot locally and replaces output only after every descriptor validates. */
usbhost_status buildDescriptorSnapshot(const RawDescriptorSet &input,
                                       DeviceDescriptor &outDescriptor,
                                       std::string &outDiagnostic);

}  // namespace usbhost::transport

#endif

#ifndef USBHOST_CORE_READ_PLAN_HPP
#define USBHOST_CORE_READ_PLAN_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace usbhost {

struct ReadChunk {
    uint32_t transferAddress;
    uint16_t transferLength;
    size_t sourceOffset;
    size_t destinationOffset;
    size_t copyLength;
};

inline std::vector<ReadChunk> planAlignedRead(uint32_t address, uint32_t length,
                                              uint16_t maximumChunk = 4096u) {
    std::vector<ReadChunk> result;
    if (length == 0 || maximumChunk < 4 || (maximumChunk & 3u) != 0) {
        return result;
    }
    const uint64_t requestedEnd = static_cast<uint64_t>(address) + length;
    const uint64_t alignedStart = address & ~UINT64_C(3);
    const uint64_t alignedEnd = (requestedEnd + 3u) & ~UINT64_C(3);

    for (uint64_t current = alignedStart; current < alignedEnd;) {
        const uint16_t transferLength = static_cast<uint16_t>(
            std::min<uint64_t>(maximumChunk, alignedEnd - current));
        const uint64_t copyStart = std::max<uint64_t>(current, address);
        const uint64_t copyEnd = std::min<uint64_t>(current + transferLength, requestedEnd);
        result.push_back(ReadChunk{
            static_cast<uint32_t>(current),
            transferLength,
            static_cast<size_t>(copyStart - current),
            static_cast<size_t>(copyStart - address),
            static_cast<size_t>(copyEnd - copyStart)});
        current += transferLength;
    }
    return result;
}

}  // namespace usbhost

#endif

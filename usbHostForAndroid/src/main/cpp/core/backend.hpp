#ifndef USBHOST_CORE_BACKEND_HPP
#define USBHOST_CORE_BACKEND_HPP

#include <cstdint>
#include <memory>

#include "core/types.hpp"

namespace usbhost {

class Backend {
public:
    virtual ~Backend() = default;
    virtual usbhost_programmer_info programmerInfo() const = 0;
    virtual Result connectTarget(usbhost_target_info &target) = 0;
    virtual Result readMemory(uint32_t address, uint8_t *destination, uint32_t length) = 0;
    virtual void close() noexcept = 0;
};

struct BackendOpenResult {
    Result result;
    std::unique_ptr<Backend> backend;
};

BackendOpenResult openStlinkBackend(
    int fd, uint16_t vendorId, uint16_t productId, uint32_t swdFrequencyKhz);

}  // namespace usbhost

#endif

#include "core/backend.hpp"

namespace usbhost {

BackendOpenResult openStlinkBackend(int, uint16_t, uint16_t, uint32_t) {
    return {Result::error(USBHOST_UNSUPPORTED_DEVICE,
                          "ST-Link transport is available only in the Android build"), nullptr};
}

}  // namespace usbhost

#include "core/backend.hpp"

#include <libusb.h>
#include <stlink.h>

extern "C" {
#include <read_write.h>
}

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "stlink/stlink_shared_transport.hpp"
#include "stlink/stlink_transport_adapter.hpp"
#include "stlink/stlink_usb_android.h"
#include "core/read_plan.hpp"

namespace usbhost {

namespace {

Result mapTransportFailure(const char *operation, usbhost_status fallback) {
    const int code = usbhost_stlink_last_transport_error();
    if (code == LIBUSB_ERROR_NO_DEVICE) {
        return Result::error(USBHOST_DISCONNECTED, std::string(operation) + ": USB device detached");
    }
    if (code == LIBUSB_ERROR_TIMEOUT) {
        return Result::error(USBHOST_TIMEOUT, std::string(operation) + ": USB transfer timed out");
    }
    if (code == LIBUSB_ERROR_ACCESS) {
        return Result::error(USBHOST_PERMISSION_DENIED,
                             std::string(operation) + ": USB access denied");
    }
    if (code != LIBUSB_SUCCESS) {
        return Result::error(USBHOST_USB_ERROR,
                             std::string(operation) + ": " + libusb_error_name(code));
    }
    return Result::error(fallback, std::string(operation) + " failed");
}

class StlinkBackend final : public Backend {
public:
    explicit StlinkBackend(stlink_t *stlink) : stlink_(stlink) {}

    ~StlinkBackend() override { close(); }

    usbhost_programmer_info programmerInfo() const override {
        usbhost_programmer_info value = emptyProgrammerInfo();
        if (stlink_) {
            value.stlink_version = stlink_->version.stlink_v;
            value.jtag_version = stlink_->version.jtag_v;
            value.swim_version = stlink_->version.swim_v;
            value.jtag_api_version = stlink_->version.jtag_api;
        }
        return value;
    }

    Result connectTarget(usbhost_target_info &target) override {
        if (!stlink_) {
            return Result::error(USBHOST_INVALID_STATE, "programmer is closed");
        }
        usbhost_stlink_reset_transport_error();
        if (stlink_target_connect(stlink_, CONNECT_HOT_PLUG) != 0) {
            if (stlink_->chip_id != 0 && stlink_->chip_id != 0x467u) {
                return Result::error(USBHOST_UNSUPPORTED_TARGET,
                                     "connected target chip id is unsupported");
            }
            return mapTransportFailure("target connection", USBHOST_TARGET_NOT_FOUND);
        }
        if (stlink_->chip_id != 0x467u) {
            return Result::error(USBHOST_UNSUPPORTED_TARGET,
                                 "connected target is not STM32G0B/G0C");
        }

        target = emptyTargetInfo();
        target.chip_id = stlink_->chip_id;
        target.flash_base = stlink_->flash_base;
        target.flash_size = stlink_->flash_size;
        target.flash_page_size = stlink_->flash_pgsz;
        target.sram_base = stlink_->sram_base;
        target.sram_size = stlink_->sram_size;
        target.target_voltage_mv = stlink_target_voltage(stlink_);
        return Result::ok();
    }

    Result readMemory(uint32_t address, uint8_t *destination, uint32_t length) override {
        if (!stlink_) {
            return Result::error(USBHOST_INVALID_STATE, "programmer is closed");
        }
        const std::vector<ReadChunk> chunks = planAlignedRead(address, length);
        for (const ReadChunk &chunk : chunks) {
            usbhost_stlink_reset_transport_error();
            if (stlink_read_mem32(stlink_, chunk.transferAddress, chunk.transferLength) != 0) {
                return mapTransportFailure("memory read", USBHOST_PROGRAMMER_ERROR);
            }
            std::memcpy(destination + chunk.destinationOffset,
                        stlink_->q_buf + chunk.sourceOffset, chunk.copyLength);
        }
        return Result::ok();
    }

    void close() noexcept override {
        if (stlink_) {
            if (stlink_->backend != nullptr && stlink_->backend->current_mode != nullptr
                    && stlink_->backend->exit_debug_mode != nullptr
                    && stlink_->backend->current_mode(stlink_) == STLINK_DEV_DEBUG_MODE) {
                // Use the transport detach directly. The upstream convenience wrapper writes
                // target debug registers; this library's hardware contract is strictly read-only.
                (void)stlink_->backend->exit_debug_mode(stlink_);
            }
            stlink_close(stlink_);
            stlink_ = nullptr;
        }
    }

private:
    stlink_t *stlink_{nullptr};
};

}  // namespace

BackendOpenResult openStlinkBackend(int fd, uint16_t vendorId, uint16_t productId,
                                    uint32_t swdFrequencyKhz) {
    usbhost_stlink_layout layout{};
    if (!usbhost_stlink_v3_layout(vendorId, productId, &layout)) {
        return {Result::error(USBHOST_UNSUPPORTED_DEVICE,
                              "USB device is not a supported ST-Link V3 debug interface"), nullptr};
    }
    const usbhost_stlink_shared_transport_api api =
        usbhost_stlink_production_transport_api();
    usbhost_stlink_transport_hooks hooks{};
    const usbhost_status transportStatus =
        usbhost_stlink_open_shared_transport(fd, &api, &hooks);
    if (transportStatus != USBHOST_OK) {
        return {Result::error(transportStatus,
                              "could not open the shared authorized USB transport"), nullptr};
    }

    usbhost_stlink_reset_transport_error();
    stlink_t *stlink = usbhost_stlink_open_transport(
        &hooks, &layout, static_cast<int32_t>(swdFrequencyKhz), nullptr);
    if (!stlink) {
        Result result = mapTransportFailure("programmer open", USBHOST_PROGRAMMER_ERROR);
        return {std::move(result), nullptr};
    }
    return {Result::ok(), std::make_unique<StlinkBackend>(stlink)};
}

}  // namespace usbhost

#include "stlink/stlink_usb_android.h"
#include "stlink/stlink_transport_adapter.hpp"

#include <cstdint>
#include <vector>

static_assert(sizeof(usbhost_stlink_layout) == 3, "endpoint layout must remain byte-sized");

namespace {
enum class Operation { Claim, Transfer, Release, Close };

struct FakeTransport {
    std::vector<Operation> operations;
    usbhost_status claim_status{USBHOST_OK};
};

usbhost_status claim(void *context, std::uint8_t interface_number) {
    auto &fake = *static_cast<FakeTransport *>(context);
    if (interface_number != 0) return USBHOST_INVALID_ARGUMENT;
    fake.operations.push_back(Operation::Claim);
    return fake.claim_status;
}

usbhost_status bulk(void *context, std::uint8_t endpoint, std::uint8_t *,
                    std::uint32_t length, std::uint32_t timeout,
                    std::uint32_t *actual) {
    auto &fake = *static_cast<FakeTransport *>(context);
    if (endpoint != 0x01u || length != 4u || timeout != 3000u)
        return USBHOST_INVALID_ARGUMENT;
    fake.operations.push_back(Operation::Transfer);
    *actual = length;
    return USBHOST_OK;
}

usbhost_status release(void *context, std::uint8_t interface_number) {
    auto &fake = *static_cast<FakeTransport *>(context);
    if (interface_number != 0) return USBHOST_INVALID_ARGUMENT;
    fake.operations.push_back(Operation::Release);
    return USBHOST_OK;
}

void close(void *context) {
    static_cast<FakeTransport *>(context)->operations.push_back(Operation::Close);
}

usbhost_stlink_transport_hooks hooksFor(FakeTransport &fake) {
    return {&fake, claim, bulk, release, close};
}
}

int runStlinkUsbContractTest() {
    usbhost_stlink_layout layout{};
    if (!usbhost_stlink_v3_layout(0x0483u, 0x374eu, &layout)) return 1;
    if (layout.request_endpoint != 0x01u || layout.reply_endpoint != 0x81u
            || layout.trace_endpoint != 0x82u) return 2;
    if (usbhost_stlink_v3_layout(0x0483u, 0x374du, &layout)) return 3;
    if (usbhost_stlink_v3_layout(0xffffu, 0x374eu, &layout)) return 4;
    if (usbhost_stlink_v3_layout(0x0483u, 0x374eu, nullptr)) return 5;

    FakeTransport fake;
    auto hooks = hooksFor(fake);
    usbhost_status open_status = USBHOST_INTERNAL_ERROR;
    auto *adapter = usbhost_stlink_transport_adapter_open(&hooks, &layout, &open_status);
    if (!adapter || open_status != USBHOST_OK) return 6;
    std::uint8_t command[4]{};
    std::uint32_t actual = 0;
    if (usbhost_stlink_transport_adapter_bulk(
            adapter, layout.request_endpoint, command, sizeof(command), 3000, &actual)
            != USBHOST_OK || actual != sizeof(command)) return 7;
    usbhost_stlink_transport_adapter_destroy(adapter);
    if (fake.operations != std::vector<Operation>{Operation::Claim, Operation::Transfer,
                                                  Operation::Release, Operation::Close}) return 8;

    FakeTransport rejected;
    rejected.claim_status = USBHOST_BUSY;
    hooks = hooksFor(rejected);
    open_status = USBHOST_OK;
    if (usbhost_stlink_transport_adapter_open(&hooks, &layout, &open_status) != nullptr
            || open_status != USBHOST_BUSY) return 9;
    if (rejected.operations != std::vector<Operation>{Operation::Claim, Operation::Close}) return 10;
    return 0;
}

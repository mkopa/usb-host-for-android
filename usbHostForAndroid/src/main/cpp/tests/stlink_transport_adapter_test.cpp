#include "stlink/stlink_transport_adapter.hpp"

#include <cstdint>

namespace {
struct State {
    int claimCalls{0};
    int transferCalls{0};
    int releaseCalls{0};
    int closeCalls{0};
    std::uint8_t endpoint{0};
    usbhost_status completion{USBHOST_OK};
    std::uint32_t actual{0};
};
usbhost_status claim(void *context, std::uint8_t interfaceNumber) {
    auto &state = *static_cast<State *>(context);
    ++state.claimCalls;
    return interfaceNumber == 0 ? USBHOST_OK : USBHOST_INVALID_ARGUMENT;
}
usbhost_status transfer(void *context, std::uint8_t endpoint, std::uint8_t *,
        std::uint32_t, std::uint32_t, std::uint32_t *actual) {
    auto &state = *static_cast<State *>(context);
    ++state.transferCalls;
    state.endpoint = endpoint;
    *actual = state.actual;
    return state.completion;
}
usbhost_status release(void *context, std::uint8_t interfaceNumber) {
    auto &state = *static_cast<State *>(context);
    ++state.releaseCalls;
    return interfaceNumber == 0 ? USBHOST_OK : USBHOST_INVALID_ARGUMENT;
}
void close(void *context) { ++static_cast<State *>(context)->closeCalls; }
}

int runStlinkTransportAdapterTest() {
    State state;
    usbhost_stlink_transport_hooks hooks{&state, claim, transfer, release, close};
    usbhost_stlink_layout layout{0x01, 0x81, 0x82};
    auto *adapter = usbhost_stlink_transport_adapter_create(&hooks, &layout);
    if (!adapter) return 1;
    if (usbhost_stlink_transport_adapter_claim(adapter) != USBHOST_OK) return 2;
    state.completion = USBHOST_TIMEOUT;
    state.actual = 3;
    std::uint8_t buffer[8]{};
    std::uint32_t actual = 0;
    if (usbhost_stlink_transport_adapter_bulk(
            adapter, 0x81, buffer, sizeof(buffer), 100, &actual) != USBHOST_TIMEOUT) return 3;
    if (actual != 3 || state.endpoint != 0x81 || state.transferCalls != 1) return 4;
    if (usbhost_stlink_transport_adapter_bulk(
            adapter, 0x83, buffer, sizeof(buffer), 100, &actual) != USBHOST_INVALID_ARGUMENT)
        return 5;
    if (state.transferCalls != 1) return 6;
    usbhost_stlink_transport_adapter_close(adapter);
    usbhost_stlink_transport_adapter_close(adapter);
    if (state.claimCalls != 1 || state.releaseCalls != 1 || state.closeCalls != 1) return 7;
    usbhost_stlink_transport_adapter_destroy(adapter);
    return 0;
}

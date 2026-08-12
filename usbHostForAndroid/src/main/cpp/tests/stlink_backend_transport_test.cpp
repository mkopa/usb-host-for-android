#include "stlink/stlink_shared_transport.hpp"

#include <cstdint>

namespace {
struct FakeSharedTransport {
    int borrowedFd{-1};
    int openCalls{0};
    int claimCalls{0};
    int transferCalls{0};
    int releaseCalls{0};
    int closeCalls{0};
    bool callerConnectionOpen{true};
};

usbhost_status openFd(void *context, int borrowedFd,
                      usbhost_transport_session *outSession) {
    auto &fake = *static_cast<FakeSharedTransport *>(context);
    fake.borrowedFd = borrowedFd;
    ++fake.openCalls;
    *outSession = 73;
    return USBHOST_OK;
}

usbhost_status claim(void *context, usbhost_transport_session session,
                     std::uint8_t interfaceNumber) {
    auto &fake = *static_cast<FakeSharedTransport *>(context);
    if (session != 73 || interfaceNumber != 0) return USBHOST_INVALID_ARGUMENT;
    ++fake.claimCalls;
    return USBHOST_OK;
}

usbhost_status bulk(void *context, usbhost_transport_session session,
                    std::uint8_t endpoint, std::uint8_t *, std::uint32_t length,
                    std::uint32_t timeout, std::uint32_t *actual) {
    auto &fake = *static_cast<FakeSharedTransport *>(context);
    if (session != 73 || endpoint != 0x01u || length != 4u || timeout != 3000u)
        return USBHOST_INVALID_ARGUMENT;
    ++fake.transferCalls;
    *actual = length;
    return USBHOST_OK;
}

usbhost_status release(void *context, usbhost_transport_session session,
                       std::uint8_t interfaceNumber) {
    auto &fake = *static_cast<FakeSharedTransport *>(context);
    if (session != 73 || interfaceNumber != 0) return USBHOST_INVALID_ARGUMENT;
    ++fake.releaseCalls;
    return USBHOST_OK;
}

usbhost_status close(void *context, usbhost_transport_session session) {
    auto &fake = *static_cast<FakeSharedTransport *>(context);
    if (session != 73) return USBHOST_INVALID_ARGUMENT;
    ++fake.closeCalls;
    return USBHOST_OK;
}
}

int runStlinkBackendTransportTest() {
    FakeSharedTransport fake;
    usbhost_stlink_shared_transport_api api{
        &fake, openFd, claim, bulk, release, close};
    usbhost_stlink_transport_hooks hooks{};
    if (usbhost_stlink_open_shared_transport(41, &api, &hooks) != USBHOST_OK) return 1;
    if (fake.borrowedFd != 41 || fake.openCalls != 1 || !fake.callerConnectionOpen) return 2;

    usbhost_stlink_layout layout{0x01u, 0x81u, 0x82u};
    usbhost_status status = USBHOST_INTERNAL_ERROR;
    auto *adapter = usbhost_stlink_transport_adapter_open(&hooks, &layout, &status);
    if (!adapter || status != USBHOST_OK || fake.claimCalls != 1) return 3;
    std::uint8_t command[4]{};
    std::uint32_t actual = 0;
    if (usbhost_stlink_transport_adapter_bulk(
            adapter, 0x01u, command, sizeof(command), 3000, &actual) != USBHOST_OK
            || actual != sizeof(command) || fake.transferCalls != 1) return 4;

    usbhost_stlink_transport_adapter_destroy(adapter);
    if (fake.releaseCalls != 1 || fake.closeCalls != 1) return 5;
    if (!fake.callerConnectionOpen) return 6;
    return 0;
}

#include "stlink/stlink_transport_adapter.hpp"

#include <new>

struct usbhost_stlink_transport_adapter {
    usbhost_stlink_transport_hooks hooks{};
    usbhost_stlink_layout layout{};
    bool claimed{false};
    bool closed{false};
};

namespace {
bool validHooks(const usbhost_stlink_transport_hooks *hooks) {
    return hooks && hooks->claim_interface && hooks->bulk_transfer
        && hooks->release_interface && hooks->close;
}

bool validEndpoint(const usbhost_stlink_transport_adapter &adapter, std::uint8_t endpoint) {
    return endpoint == adapter.layout.request_endpoint
        || endpoint == adapter.layout.reply_endpoint
        || endpoint == adapter.layout.trace_endpoint;
}
}

extern "C" usbhost_stlink_transport_adapter *usbhost_stlink_transport_adapter_create(
        const usbhost_stlink_transport_hooks *hooks, const usbhost_stlink_layout *layout) {
    if (!validHooks(hooks) || !layout || layout->request_endpoint == 0
            || layout->reply_endpoint == 0 || layout->trace_endpoint == 0)
        return nullptr;
    auto *adapter = new (std::nothrow) usbhost_stlink_transport_adapter;
    if (!adapter) return nullptr;
    adapter->hooks = *hooks;
    adapter->layout = *layout;
    return adapter;
}

extern "C" usbhost_stlink_transport_adapter *usbhost_stlink_transport_adapter_open(
        const usbhost_stlink_transport_hooks *hooks, const usbhost_stlink_layout *layout,
        usbhost_status *out_status) {
    if (!out_status) return nullptr;
    *out_status = USBHOST_INVALID_ARGUMENT;
    auto *adapter = usbhost_stlink_transport_adapter_create(hooks, layout);
    if (!adapter) return nullptr;
    *out_status = usbhost_stlink_transport_adapter_claim(adapter);
    if (*out_status != USBHOST_OK) {
        usbhost_stlink_transport_adapter_destroy(adapter);
        return nullptr;
    }
    return adapter;
}

extern "C" usbhost_status usbhost_stlink_transport_adapter_claim(
        usbhost_stlink_transport_adapter *adapter) {
    if (!adapter || adapter->closed) return USBHOST_INVALID_STATE;
    if (adapter->claimed) return USBHOST_OK;
    const usbhost_status status = adapter->hooks.claim_interface(adapter->hooks.context, 0);
    if (status == USBHOST_OK) adapter->claimed = true;
    return status;
}

extern "C" usbhost_status usbhost_stlink_transport_adapter_bulk(
        usbhost_stlink_transport_adapter *adapter, std::uint8_t endpoint,
        std::uint8_t *buffer, std::uint32_t length, std::uint32_t timeout,
        std::uint32_t *actual) {
    if (!actual) return USBHOST_INVALID_ARGUMENT;
    *actual = 0;
    if (!adapter || adapter->closed || !adapter->claimed) return USBHOST_INVALID_STATE;
    if (!validEndpoint(*adapter, endpoint) || (length != 0 && !buffer)
            || length > USBHOST_MAX_READ_SIZE || timeout < 1 || timeout > 60000)
        return USBHOST_INVALID_ARGUMENT;
    const usbhost_status status = adapter->hooks.bulk_transfer(
        adapter->hooks.context, endpoint, buffer, length, timeout, actual);
    if (*actual > length) {
        *actual = 0;
        return USBHOST_INTERNAL_ERROR;
    }
    return status;
}

extern "C" void usbhost_stlink_transport_adapter_close(
        usbhost_stlink_transport_adapter *adapter) {
    if (!adapter || adapter->closed) return;
    adapter->closed = true;
    if (adapter->claimed) {
        (void)adapter->hooks.release_interface(adapter->hooks.context, 0);
        adapter->claimed = false;
    }
    adapter->hooks.close(adapter->hooks.context);
}

extern "C" void usbhost_stlink_transport_adapter_destroy(
        usbhost_stlink_transport_adapter *adapter) {
    if (!adapter) return;
    usbhost_stlink_transport_adapter_close(adapter);
    delete adapter;
}

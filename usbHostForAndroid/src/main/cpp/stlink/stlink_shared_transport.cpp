#include "stlink/stlink_shared_transport.hpp"

#include <new>

namespace {
struct SharedTransportContext {
    usbhost_stlink_shared_transport_api api{};
    usbhost_transport_session session{USBHOST_TRANSPORT_INVALID_SESSION};
};

bool validApi(const usbhost_stlink_shared_transport_api *api) {
    return api && api->open_fd && api->claim_interface && api->bulk_transfer
        && api->release_interface && api->close;
}

usbhost_status claimInterface(void *opaque, std::uint8_t interfaceNumber) {
    auto &context = *static_cast<SharedTransportContext *>(opaque);
    return context.api.claim_interface(
        context.api.context, context.session, interfaceNumber);
}

usbhost_status bulkTransfer(
        void *opaque, std::uint8_t endpointAddress, std::uint8_t *buffer,
        std::uint32_t length, std::uint32_t timeoutMilliseconds,
        std::uint32_t *outActualLength) {
    auto &context = *static_cast<SharedTransportContext *>(opaque);
    return context.api.bulk_transfer(
        context.api.context, context.session, endpointAddress, buffer,
        length, timeoutMilliseconds, outActualLength);
}

usbhost_status releaseInterface(void *opaque, std::uint8_t interfaceNumber) {
    auto &context = *static_cast<SharedTransportContext *>(opaque);
    return context.api.release_interface(
        context.api.context, context.session, interfaceNumber);
}

void closeTransport(void *opaque) {
    auto *context = static_cast<SharedTransportContext *>(opaque);
    if (!context) return;
    (void)context->api.close(context->api.context, context->session);
    delete context;
}

usbhost_status productionOpen(
        void *, int borrowedFd, usbhost_transport_session *outSession) {
    return usbhost_transport_open_fd(borrowedFd, outSession);
}
usbhost_status productionClaim(
        void *, usbhost_transport_session session, std::uint8_t interfaceNumber) {
    return usbhost_transport_claim_interface(session, interfaceNumber);
}
usbhost_status productionBulk(
        void *, usbhost_transport_session session, std::uint8_t endpointAddress,
        std::uint8_t *buffer, std::uint32_t length, std::uint32_t timeoutMilliseconds,
        std::uint32_t *outActualLength) {
    return usbhost_transport_bulk_transfer(
        session, endpointAddress, buffer, length, timeoutMilliseconds, outActualLength);
}
usbhost_status productionRelease(
        void *, usbhost_transport_session session, std::uint8_t interfaceNumber) {
    return usbhost_transport_release_interface(session, interfaceNumber);
}
usbhost_status productionClose(void *, usbhost_transport_session session) {
    return usbhost_transport_close(session);
}
}

extern "C" usbhost_stlink_shared_transport_api
usbhost_stlink_production_transport_api(void) {
    return {nullptr, productionOpen, productionClaim, productionBulk,
            productionRelease, productionClose};
}

extern "C" usbhost_status usbhost_stlink_open_shared_transport(
        int borrowedFd, const usbhost_stlink_shared_transport_api *api,
        usbhost_stlink_transport_hooks *outHooks) {
    if (borrowedFd < 0 || !validApi(api) || !outHooks)
        return USBHOST_INVALID_ARGUMENT;
    *outHooks = {};
    auto *context = new (std::nothrow) SharedTransportContext;
    if (!context) return USBHOST_INTERNAL_ERROR;
    context->api = *api;
    const usbhost_status status = api->open_fd(
        api->context, borrowedFd, &context->session);
    if (status != USBHOST_OK || context->session == USBHOST_TRANSPORT_INVALID_SESSION) {
        if (context->session != USBHOST_TRANSPORT_INVALID_SESSION)
            (void)api->close(api->context, context->session);
        if (status == USBHOST_OK) {
            delete context;
            return USBHOST_INTERNAL_ERROR;
        }
        delete context;
        return status;
    }
    *outHooks = {context, claimInterface, bulkTransfer, releaseInterface, closeTransport};
    return USBHOST_OK;
}

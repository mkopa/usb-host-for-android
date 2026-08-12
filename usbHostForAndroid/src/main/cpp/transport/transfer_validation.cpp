#include "transport/transfer_validation.hpp"

namespace usbhost::transport {
namespace {

TransportError invalid(const char *diagnostic) {
    return {USBHOST_INVALID_ARGUMENT, 0, diagnostic};
}

TransportError validateCommon(const BufferSlice &slice, std::uint32_t capacity,
                              std::uint32_t maximumLength, std::uint32_t timeout) {
    if (!slice.fits(capacity) || slice.length > maximumLength) {
        return invalid("transfer buffer slice is outside its allowed bounds");
    }
    if (timeout < USBHOST_TRANSPORT_MIN_TIMEOUT_MS
            || timeout > USBHOST_TRANSPORT_MAX_TIMEOUT_MS) {
        return invalid("transfer timeout is outside its allowed bounds");
    }
    return {};
}

bool validDirection(Direction direction) {
    return direction == Direction::In || direction == Direction::Out;
}

Direction addressDirection(std::uint8_t address) {
    return (address & 0x80u) == 0 ? Direction::Out : Direction::In;
}

}  // namespace

TransportError validateControlTransfer(const ControlRequest &request,
                                       std::uint32_t bufferCapacity,
                                       SnapshotGeneration activeGeneration) {
    const TransportError common = validateCommon(request.buffer, bufferCapacity,
        USBHOST_TRANSPORT_MAX_CONTROL_LENGTH, request.timeoutMilliseconds);
    if (common.status != USBHOST_OK) return common;
    if (!activeGeneration.isValid() || request.generation != activeGeneration) {
        return {USBHOST_INVALID_STATE, 0, "control request snapshot is stale"};
    }
    if (!validDirection(request.direction)
            || request.direction != addressDirection(request.requestType)) {
        return invalid("control request direction does not match bmRequestType");
    }
    return {};
}

TransportError validateEndpointTransfer(const EndpointTransferRequest &request,
                                        std::uint32_t bufferCapacity,
                                        const EndpointDescriptor &endpoint) {
    const TransportError common = validateCommon(request.buffer, bufferCapacity,
        USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, request.timeoutMilliseconds);
    if (common.status != USBHOST_OK) return common;
    if (!endpoint.generation.isValid() || request.generation != endpoint.generation) {
        return {USBHOST_INVALID_STATE, 0, "endpoint snapshot is stale"};
    }
    if (request.transferType == TransferType::Isochronous
            || endpoint.transferType == TransferType::Isochronous) {
        return {USBHOST_UNSUPPORTED_OPERATION, 0,
                "isochronous transfer execution is not supported"};
    }
    if ((request.transferType != TransferType::Bulk
                && request.transferType != TransferType::Interrupt)
            || request.transferType != endpoint.transferType) {
        return invalid("endpoint transfer type does not match the descriptor");
    }
    if (!validDirection(request.direction) || request.endpointAddress != endpoint.address
            || request.direction != endpoint.direction
            || request.direction != addressDirection(request.endpointAddress)) {
        return invalid("endpoint address or direction does not match the descriptor");
    }
    return {};
}

}  // namespace usbhost::transport

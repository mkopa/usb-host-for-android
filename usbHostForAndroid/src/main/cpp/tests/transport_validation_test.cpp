#include "transport/transfer_validation.hpp"

#include <cstdint>
#include <limits>

namespace {
using namespace usbhost::transport;

int failures;
#define CHECK_VALIDATION(value) do { if (!(value)) ++failures; } while (0)

ControlRequest validControl() {
    ControlRequest value;
    value.requestType = 0x80;
    value.direction = Direction::In;
    value.buffer = {4, 8};
    value.timeoutMilliseconds = 100;
    value.generation = SnapshotGeneration::fromRaw(3);
    return value;
}

void controlBoundaries() {
    ControlRequest value = validControl();
    CHECK_VALIDATION(validateControlTransfer(value, 12, value.generation).status == USBHOST_OK);
    value.buffer = {std::numeric_limits<std::uint32_t>::max(), 2};
    CHECK_VALIDATION(validateControlTransfer(value,
        std::numeric_limits<std::uint32_t>::max(), value.generation).status
        == USBHOST_INVALID_ARGUMENT);
    value = validControl(); value.direction = Direction::Out;
    CHECK_VALIDATION(validateControlTransfer(value, 12, value.generation).status
        == USBHOST_INVALID_ARGUMENT);
    value = validControl(); value.buffer.length = USBHOST_TRANSPORT_MAX_CONTROL_LENGTH + 1;
    CHECK_VALIDATION(validateControlTransfer(value, value.buffer.length, value.generation).status
        == USBHOST_INVALID_ARGUMENT);
    value = validControl(); value.timeoutMilliseconds = 0;
    CHECK_VALIDATION(validateControlTransfer(value, 12, value.generation).status
        == USBHOST_INVALID_ARGUMENT);
    value = validControl(); value.timeoutMilliseconds = USBHOST_TRANSPORT_MAX_TIMEOUT_MS + 1;
    CHECK_VALIDATION(validateControlTransfer(value, 12, value.generation).status
        == USBHOST_INVALID_ARGUMENT);
    value = validControl();
    CHECK_VALIDATION(validateControlTransfer(
        value, 12, SnapshotGeneration::fromRaw(4)).status == USBHOST_INVALID_STATE);
}

void endpointBoundaries() {
    EndpointDescriptor endpoint;
    endpoint.address = 0x81;
    endpoint.number = 1;
    endpoint.direction = Direction::In;
    endpoint.transferType = TransferType::Bulk;
    endpoint.generation = SnapshotGeneration::fromRaw(8);
    EndpointTransferRequest request;
    request.transferType = TransferType::Bulk;
    request.endpointAddress = 0x81;
    request.direction = Direction::In;
    request.buffer = {0, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH};
    request.timeoutMilliseconds = USBHOST_TRANSPORT_MAX_TIMEOUT_MS;
    request.generation = endpoint.generation;
    CHECK_VALIDATION(validateEndpointTransfer(
        request, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, endpoint).status == USBHOST_OK);
    request.transferType = TransferType::Interrupt;
    CHECK_VALIDATION(validateEndpointTransfer(
        request, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, endpoint).status
        == USBHOST_INVALID_ARGUMENT);
    request.transferType = TransferType::Isochronous;
    CHECK_VALIDATION(validateEndpointTransfer(
        request, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, endpoint).status
        == USBHOST_UNSUPPORTED_OPERATION);
    request.transferType = TransferType::Bulk;
    request.endpointAddress = 0x01;
    CHECK_VALIDATION(validateEndpointTransfer(
        request, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, endpoint).status
        == USBHOST_INVALID_ARGUMENT);
    request.endpointAddress = 0x81;
    request.generation = SnapshotGeneration::fromRaw(9);
    CHECK_VALIDATION(validateEndpointTransfer(
        request, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH, endpoint).status
        == USBHOST_INVALID_STATE);
}
}

int runTransportValidationTest() {
    failures = 0;
    controlBoundaries();
    endpointBoundaries();
    return failures;
}

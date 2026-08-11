#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

#include "transport/types.hpp"

namespace {

int failures = 0;

#define CHECK_TYPE(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

void generationTest() {
    using usbhost::transport::SnapshotGeneration;
    const SnapshotGeneration invalid;
    CHECK_TYPE(!invalid.isValid());
    CHECK_TYPE(invalid.next() == SnapshotGeneration::initial());
    CHECK_TYPE(SnapshotGeneration::initial().value() == UINT64_C(1));
    CHECK_TYPE(SnapshotGeneration::fromRaw(UINT64_C(41)).next().value() == UINT64_C(42));
    CHECK_TYPE(SnapshotGeneration::fromRaw(std::numeric_limits<std::uint64_t>::max()).next()
               == SnapshotGeneration::initial());
}

void bufferSliceTest() {
    using usbhost::transport::BufferSlice;
    CHECK_TYPE((BufferSlice{0, 0}.fits(0)));
    CHECK_TYPE((BufferSlice{4, 8}.fits(12)));
    CHECK_TYPE((!BufferSlice{4, 9}.fits(12)));
    CHECK_TYPE((!BufferSlice{std::numeric_limits<std::uint32_t>::max(), 2}.fits(
        std::numeric_limits<std::uint32_t>::max())));
}

void descriptorOwnershipTest() {
    using namespace usbhost::transport;
    std::vector<std::uint8_t> source{3, 0x21, 0x7f};
    AdditionalDescriptor additional{0x21, source};
    source[2] = 0;
    CHECK_TYPE(additional.type == 0x21);
    CHECK_TYPE(additional.bytes.size() == 3);
    CHECK_TYPE(additional.bytes[2] == 0x7f);

    EndpointDescriptor endpoint;
    endpoint.address = 0x81;
    endpoint.number = 1;
    endpoint.direction = Direction::In;
    endpoint.transferType = TransferType::Bulk;
    endpoint.maximumPacketSize = 512;
    endpoint.additionalDescriptors.push_back(additional);
    CHECK_TYPE(endpoint.additionalDescriptors.front().bytes[2] == 0x7f);

    static_assert(std::is_copy_constructible_v<DeviceDescriptor>);
    static_assert(std::is_move_constructible_v<DeviceDescriptor>);
}

void requestAndResultTest() {
    using namespace usbhost::transport;
    static_assert(static_cast<std::uint8_t>(Direction::Out) == 0);
    static_assert(static_cast<std::uint8_t>(Direction::In) == 1);
    static_assert(static_cast<std::uint8_t>(TransferType::Control) == 0);
    static_assert(static_cast<std::uint8_t>(TransferType::Isochronous) == 1);
    static_assert(static_cast<std::uint8_t>(TransferType::Bulk) == 2);
    static_assert(static_cast<std::uint8_t>(TransferType::Interrupt) == 3);

    ControlRequest control;
    control.requestType = 0x80;
    control.direction = Direction::In;
    control.buffer = {0, USBHOST_TRANSPORT_MAX_CONTROL_LENGTH};
    control.timeoutMilliseconds = USBHOST_TRANSPORT_MAX_TIMEOUT_MS;
    control.generation = SnapshotGeneration::initial();
    CHECK_TYPE(control.buffer.length == UINT32_C(65535));

    EndpointTransferRequest endpoint;
    endpoint.transferType = TransferType::Interrupt;
    endpoint.endpointAddress = 0x82;
    endpoint.direction = Direction::In;
    endpoint.buffer = {0, USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH};
    endpoint.timeoutMilliseconds = USBHOST_TRANSPORT_MIN_TIMEOUT_MS;
    endpoint.generation = SnapshotGeneration::fromRaw(7);
    CHECK_TYPE(endpoint.buffer.length == UINT32_C(1048576));

    TransferResult result{USBHOST_CANCELLED, 17, "cancelled"};
    CHECK_TYPE(!result.isOk());
    CHECK_TYPE(result.actualLength == 17);
    CHECK_TYPE(TransferResult{}.isOk());
}

void sessionStateTest() {
    using usbhost::transport::SessionState;
    CHECK_TYPE(SessionState::Opening != SessionState::Open);
    CHECK_TYPE(SessionState::Open != SessionState::Closing);
    CHECK_TYPE(SessionState::Closing != SessionState::Closed);
    CHECK_TYPE(SessionState::Failed != SessionState::Closed);
}

}  // namespace

int runTransportTypesTest() {
    failures = 0;
    generationTest();
    bufferSliceTest();
    descriptorOwnershipTest();
    requestAndResultTest();
    sessionStateTest();
    return failures;
}

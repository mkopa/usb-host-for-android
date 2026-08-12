#ifndef USBHOST_TRANSPORT_TRANSFER_VALIDATION_HPP
#define USBHOST_TRANSPORT_TRANSFER_VALIDATION_HPP

#include <cstdint>

#include "transport/error.hpp"
#include "transport/types.hpp"

namespace usbhost::transport {

TransportError validateControlTransfer(const ControlRequest &request,
                                       std::uint32_t bufferCapacity,
                                       SnapshotGeneration activeGeneration);

TransportError validateEndpointTransfer(const EndpointTransferRequest &request,
                                        std::uint32_t bufferCapacity,
                                        const EndpointDescriptor &endpoint);

}  // namespace usbhost::transport

#endif

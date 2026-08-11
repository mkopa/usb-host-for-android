#ifndef USBHOST_TRANSPORT_BACKEND_HPP
#define USBHOST_TRANSPORT_BACKEND_HPP

#include <cstdint>
#include <functional>
#include <string>

#include "transport/error.hpp"
#include "transport/types.hpp"

namespace usbhost::transport {

using OperationId = std::uint64_t;
constexpr OperationId kInvalidOperationId = UINT64_C(0);

struct MutableBufferView {
    std::uint8_t *data{nullptr};
    std::uint32_t capacity{0};

    bool isValid() const noexcept {
        return data != nullptr || capacity == 0;
    }
};

struct BackendCompletion {
    BackendStatus status{BackendStatus::InternalFailure};
    std::uint32_t actualLength{0};
    std::string diagnostic;
};

using CompletionCallback = std::function<void(BackendCompletion)>;

/**
 * Portable asynchronous USB boundary.
 *
 * A successful submit returns a non-zero operation ID and invokes its callback exactly once.
 * The caller keeps MutableBufferView storage valid until that callback begins. The backend must
 * retain neither the address nor the callback after completion. Rejected submits return zero but
 * still invoke the callback exactly once with the rejection status.
 */
class UsbBackend {
public:
    virtual ~UsbBackend() = default;

    virtual const DeviceDescriptor &deviceDescriptor() const noexcept = 0;
    virtual BackendStatus selectConfiguration(std::uint8_t configurationValue) = 0;
    virtual BackendStatus claimInterface(std::uint8_t interfaceNumber) = 0;
    virtual BackendStatus selectAlternateSetting(std::uint8_t interfaceNumber,
                                                  std::uint8_t alternateSetting) = 0;
    virtual BackendStatus releaseInterface(std::uint8_t interfaceNumber) = 0;

    virtual OperationId submitControl(const ControlRequest &request,
                                      MutableBufferView buffer,
                                      CompletionCallback completion) = 0;
    virtual OperationId submitEndpoint(const EndpointTransferRequest &request,
                                       MutableBufferView buffer,
                                       CompletionCallback completion) = 0;
    virtual bool cancel(OperationId operation) = 0;
    virtual void close() noexcept = 0;
};

}  // namespace usbhost::transport

#endif

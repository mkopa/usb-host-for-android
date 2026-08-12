#ifndef USBHOST_TRANSPORT_SESSION_HPP
#define USBHOST_TRANSPORT_SESSION_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "transport/backend.hpp"
#include "transport/error.hpp"
#include "transport/registry.hpp"

namespace usbhost::transport {

struct InterfaceClaimToken {
    std::uint8_t interfaceNumber{0};
    std::uint64_t claimGeneration{0};
    SnapshotGeneration snapshotGeneration;

    bool isValid() const noexcept {
        return claimGeneration != 0 && snapshotGeneration.isValid();
    }
};

class AuthorizedBackendFactory {
public:
    virtual ~AuthorizedBackendFactory() = default;

    /** The supplied descriptor is borrowed; an implementation must duplicate it before ownership. */
    virtual BackendStatus openAuthorizedFileDescriptor(
        int borrowedFileDescriptor,
        std::unique_ptr<UsbBackend> &outBackend,
        std::string &outDiagnostic) = 0;
};

class TransportSession final : public RegistryEntry {
public:
    static std::shared_ptr<TransportSession> open(int borrowedFileDescriptor,
                                                   AuthorizedBackendFactory &factory,
                                                   TransportError &outError);

    ~TransportSession() override;
    TransportSession(const TransportSession &) = delete;
    TransportSession &operator=(const TransportSession &) = delete;

    usbhost_status close();
    TransportError selectConfiguration(std::uint8_t configurationValue);
    TransportError claimInterface(std::uint8_t interfaceNumber,
                                  InterfaceClaimToken &outToken);
    TransportError selectAlternateSetting(InterfaceClaimToken &token,
                                          std::uint8_t alternateSetting);
    TransportError releaseInterface(const InterfaceClaimToken &token);
    TransportError validateEndpoint(const InterfaceClaimToken &token,
                                    const EndpointDescriptor &endpoint) const;
    TransferResult controlTransfer(const ControlRequest &request,
                                   MutableBufferView buffer);
    TransferResult endpointTransfer(const InterfaceClaimToken &token,
                                    const EndpointDescriptor &endpoint,
                                    const EndpointTransferRequest &request,
                                    MutableBufferView buffer);
    SessionState state() const;
    DeviceDescriptor descriptorSnapshot() const;

private:
    struct ActiveTransfer;
    TransportSession(std::unique_ptr<UsbBackend> backend, DeviceDescriptor descriptor);

    mutable std::mutex mutex_;
    std::condition_variable closedCondition_;
    std::unique_ptr<UsbBackend> backend_;
    DeviceDescriptor descriptor_;
    std::unordered_map<std::uint8_t, InterfaceClaimToken> claims_;
    std::uint64_t nextClaimGeneration_{1};
    std::shared_ptr<ActiveTransfer> activeTransfer_;
    SessionState state_{SessionState::Opening};
};

}  // namespace usbhost::transport

#endif

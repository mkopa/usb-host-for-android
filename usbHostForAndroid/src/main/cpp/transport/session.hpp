#ifndef USBHOST_TRANSPORT_SESSION_HPP
#define USBHOST_TRANSPORT_SESSION_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "transport/backend.hpp"
#include "transport/error.hpp"
#include "transport/registry.hpp"

namespace usbhost::transport {

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
    SessionState state() const;
    DeviceDescriptor descriptorSnapshot() const;

private:
    TransportSession(std::unique_ptr<UsbBackend> backend, DeviceDescriptor descriptor);

    mutable std::mutex mutex_;
    std::condition_variable closedCondition_;
    std::unique_ptr<UsbBackend> backend_;
    DeviceDescriptor descriptor_;
    SessionState state_{SessionState::Opening};
};

}  // namespace usbhost::transport

#endif

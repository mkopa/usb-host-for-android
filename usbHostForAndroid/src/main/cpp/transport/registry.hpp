#ifndef USBHOST_TRANSPORT_REGISTRY_HPP
#define USBHOST_TRANSPORT_REGISTRY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "usbhost/transport.h"

namespace usbhost::transport {

using TransportHandle = usbhost_transport_session;
constexpr TransportHandle kInvalidTransportHandle = USBHOST_TRANSPORT_INVALID_SESSION;

class RegistryEntry {
public:
    virtual ~RegistryEntry() = default;
};

/**
 * Process-local owner of opaque transport session handles.
 *
 * Handles encode a non-zero slot and generation. Retiring an entry advances the generation before
 * its slot can be reused, so stale or fabricated handles never resolve to a replacement entry.
 * Returned shared pointers keep an entry alive after the registry lock is released.
 */
class TransportRegistry final {
public:
    TransportRegistry() = default;
    TransportRegistry(const TransportRegistry &) = delete;
    TransportRegistry &operator=(const TransportRegistry &) = delete;

    TransportHandle insert(std::shared_ptr<RegistryEntry> entry);
    std::shared_ptr<RegistryEntry> find(TransportHandle handle) const;
    std::shared_ptr<RegistryEntry> retire(TransportHandle handle);
    std::size_t size() const;

private:
    struct Slot {
        std::uint32_t generation{1};
        std::shared_ptr<RegistryEntry> entry;
    };

    static TransportHandle encode(std::uint32_t slotIndex,
                                  std::uint32_t generation) noexcept;
    static bool decode(TransportHandle handle,
                       std::uint32_t &slotIndex,
                       std::uint32_t &generation) noexcept;
    static std::uint32_t nextGeneration(std::uint32_t generation) noexcept;

    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> freeSlots_;
    std::size_t activeCount_{0};
};

TransportRegistry &globalTransportRegistry() noexcept;

}  // namespace usbhost::transport

#endif

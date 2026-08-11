#include "transport/registry.hpp"

#include <limits>
#include <utility>

namespace usbhost::transport {

namespace {

constexpr std::uint64_t kSlotMask = UINT64_C(0xffffffff);
constexpr unsigned int kGenerationShift = 32;

}  // namespace

TransportHandle TransportRegistry::insert(std::shared_ptr<RegistryEntry> entry) {
    if (!entry) {
        return kInvalidTransportHandle;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::uint32_t slotIndex = 0;
    if (!freeSlots_.empty()) {
        slotIndex = freeSlots_.back();
        freeSlots_.pop_back();
    } else {
        if (slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
            return kInvalidTransportHandle;
        }
        slotIndex = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();
    }

    Slot &slot = slots_[slotIndex];
    slot.entry = std::move(entry);
    ++activeCount_;
    return encode(slotIndex, slot.generation);
}

std::shared_ptr<RegistryEntry> TransportRegistry::find(TransportHandle handle) const {
    std::uint32_t slotIndex = 0;
    std::uint32_t generation = 0;
    if (!decode(handle, slotIndex, generation)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (slotIndex >= slots_.size()) {
        return nullptr;
    }
    const Slot &slot = slots_[slotIndex];
    return slot.generation == generation ? slot.entry : nullptr;
}

std::shared_ptr<RegistryEntry> TransportRegistry::retire(TransportHandle handle) {
    std::uint32_t slotIndex = 0;
    std::uint32_t generation = 0;
    if (!decode(handle, slotIndex, generation)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (slotIndex >= slots_.size()) {
        return nullptr;
    }
    Slot &slot = slots_[slotIndex];
    if (!slot.entry || slot.generation != generation) {
        return nullptr;
    }

    std::shared_ptr<RegistryEntry> retired = std::move(slot.entry);
    slot.generation = nextGeneration(slot.generation);
    freeSlots_.push_back(slotIndex);
    --activeCount_;
    return retired;
}

std::size_t TransportRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeCount_;
}

TransportHandle TransportRegistry::encode(std::uint32_t slotIndex,
                                          std::uint32_t generation) noexcept {
    const std::uint64_t encodedSlot = static_cast<std::uint64_t>(slotIndex) + UINT64_C(1);
    return (static_cast<std::uint64_t>(generation) << kGenerationShift) | encodedSlot;
}

bool TransportRegistry::decode(TransportHandle handle,
                               std::uint32_t &slotIndex,
                               std::uint32_t &generation) noexcept {
    if (handle == kInvalidTransportHandle) {
        return false;
    }
    const std::uint64_t encodedSlot = handle & kSlotMask;
    generation = static_cast<std::uint32_t>(handle >> kGenerationShift);
    if (encodedSlot == 0 || generation == 0) {
        return false;
    }
    slotIndex = static_cast<std::uint32_t>(encodedSlot - UINT64_C(1));
    return true;
}

std::uint32_t TransportRegistry::nextGeneration(std::uint32_t generation) noexcept {
    ++generation;
    return generation == 0 ? 1 : generation;
}

TransportRegistry &globalTransportRegistry() noexcept {
    static TransportRegistry registry;
    return registry;
}

}  // namespace usbhost::transport

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

#include "transport/registry.hpp"

namespace {

std::atomic<int> failures{0};

#define CHECK_REGISTRY(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

class TestEntry final : public usbhost::transport::RegistryEntry {
public:
    explicit TestEntry(int value) : value_(value) {}

    int value() const noexcept {
        return value_;
    }

private:
    int value_;
};

void lifecycleAndGenerationTest() {
    using namespace usbhost::transport;
    TransportRegistry registry;

    CHECK_REGISTRY(registry.insert(nullptr) == kInvalidTransportHandle);
    const auto firstEntry = std::make_shared<TestEntry>(7);
    const TransportHandle first = registry.insert(firstEntry);
    CHECK_REGISTRY(first != kInvalidTransportHandle);
    CHECK_REGISTRY(registry.size() == 1);
    CHECK_REGISTRY(registry.find(first) == firstEntry);
    CHECK_REGISTRY(registry.find(kInvalidTransportHandle) == nullptr);

    CHECK_REGISTRY(registry.retire(first) == firstEntry);
    CHECK_REGISTRY(registry.size() == 0);
    CHECK_REGISTRY(registry.find(first) == nullptr);
    CHECK_REGISTRY(registry.retire(first) == nullptr);

    const auto replacement = std::make_shared<TestEntry>(8);
    const TransportHandle second = registry.insert(replacement);
    CHECK_REGISTRY(second != kInvalidTransportHandle);
    CHECK_REGISTRY(second != first);
    CHECK_REGISTRY(registry.find(first) == nullptr);
    CHECK_REGISTRY(registry.find(second) == replacement);

    const TransportHandle fabricated = second ^ (UINT64_C(1) << 32);
    CHECK_REGISTRY(registry.find(fabricated) == nullptr);
    CHECK_REGISTRY(registry.retire(fabricated) == nullptr);
    CHECK_REGISTRY(registry.find(second) == replacement);
}

void concurrentAccessTest() {
    using namespace usbhost::transport;
    constexpr int threadCount = 8;
    constexpr int entriesPerThread = 128;
    TransportRegistry registry;
    std::atomic<bool> start{false};
    std::vector<std::vector<TransportHandle>> handles(threadCount);
    std::vector<std::thread> workers;

    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        workers.emplace_back([&, threadIndex] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            auto &owned = handles[threadIndex];
            owned.reserve(entriesPerThread);
            for (int index = 0; index < entriesPerThread; ++index) {
                const auto entry = std::make_shared<TestEntry>(threadIndex * entriesPerThread + index);
                const TransportHandle handle = registry.insert(entry);
                if (handle == kInvalidTransportHandle || registry.find(handle) != entry) {
                    ++failures;
                }
                owned.push_back(handle);
            }
        });
    }
    start.store(true, std::memory_order_release);
    for (auto &worker : workers) {
        worker.join();
    }

    std::unordered_set<TransportHandle> unique;
    for (const auto &owned : handles) {
        unique.insert(owned.begin(), owned.end());
    }
    CHECK_REGISTRY(unique.size() == static_cast<std::size_t>(threadCount * entriesPerThread));
    CHECK_REGISTRY(registry.size() == unique.size());

    workers.clear();
    start.store(false, std::memory_order_release);
    for (const auto &owned : handles) {
        workers.emplace_back([&, owned] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (const TransportHandle handle : owned) {
                if (!registry.retire(handle)) {
                    ++failures;
                }
            }
        });
    }
    start.store(true, std::memory_order_release);
    for (auto &worker : workers) {
        worker.join();
    }
    CHECK_REGISTRY(registry.size() == 0);
    for (const TransportHandle handle : unique) {
        CHECK_REGISTRY(registry.find(handle) == nullptr);
    }
}

}  // namespace

int runTransportRegistryTest() {
    failures.store(0, std::memory_order_relaxed);
    lifecycleAndGenerationTest();
    concurrentAccessTest();
    return failures.load(std::memory_order_relaxed);
}

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "android/libusb_runtime.hpp"

namespace {

std::atomic<int> failures{0};

#define CHECK_RUNTIME(condition) do { \
    if (!(condition)) { \
        ++failures; \
    } \
} while (0)

struct FakeLibusb {
    std::mutex mutex;
    std::condition_variable condition;
    int initializeCalls{0};
    int eventCalls{0};
    int interruptCalls{0};
    int exitCalls{0};
    int initializeResult{0};
    bool noDiscovery{false};
    bool interrupted{false};
};

int initialize(void *userData, bool noDiscovery, void **outContext) {
    auto &fake = *static_cast<FakeLibusb *>(userData);
    std::lock_guard<std::mutex> lock(fake.mutex);
    ++fake.initializeCalls;
    fake.noDiscovery = noDiscovery;
    if (fake.initializeResult != 0) {
        *outContext = nullptr;
        return fake.initializeResult;
    }
    *outContext = &fake;
    return 0;
}

int handleEvents(void *userData, void *context) {
    auto &fake = *static_cast<FakeLibusb *>(userData);
    CHECK_RUNTIME(context == &fake);
    std::unique_lock<std::mutex> lock(fake.mutex);
    ++fake.eventCalls;
    fake.condition.notify_all();
    fake.condition.wait(lock, [&fake] { return fake.interrupted; });
    fake.interrupted = false;
    return 0;
}

void interruptEvents(void *userData, void *context) {
    auto &fake = *static_cast<FakeLibusb *>(userData);
    CHECK_RUNTIME(context == &fake);
    std::lock_guard<std::mutex> lock(fake.mutex);
    ++fake.interruptCalls;
    fake.interrupted = true;
    fake.condition.notify_all();
}

void exitContext(void *userData, void *context) {
    auto &fake = *static_cast<FakeLibusb *>(userData);
    CHECK_RUNTIME(context == &fake);
    std::lock_guard<std::mutex> lock(fake.mutex);
    ++fake.exitCalls;
}

usbhost::android::LibusbRuntimeHooks hooks(FakeLibusb &fake) {
    return {&fake, initialize, handleEvents, interruptEvents, exitContext};
}

bool waitForEvents(FakeLibusb &fake, int expected) {
    std::unique_lock<std::mutex> lock(fake.mutex);
    return fake.condition.wait_for(lock, std::chrono::seconds(2),
                                   [&fake, expected] { return fake.eventCalls >= expected; });
}

void referenceCountingAndLifecycleTest() {
    using namespace usbhost::android;
    FakeLibusb fake;
    LibusbRuntimePool pool(hooks(fake));
    int error = 123;
    auto first = pool.acquire(error);
    CHECK_RUNTIME(first != nullptr);
    CHECK_RUNTIME(error == 0);
    CHECK_RUNTIME(first->nativeContext() == &fake);
    CHECK_RUNTIME(waitForEvents(fake, 1));

    auto second = pool.acquire(error);
    CHECK_RUNTIME(second == first);
    {
        std::lock_guard<std::mutex> lock(fake.mutex);
        CHECK_RUNTIME(fake.initializeCalls == 1);
        CHECK_RUNTIME(fake.noDiscovery);
        CHECK_RUNTIME(fake.exitCalls == 0);
    }

    first.reset();
    {
        std::lock_guard<std::mutex> lock(fake.mutex);
        CHECK_RUNTIME(fake.exitCalls == 0);
    }
    second.reset();
    {
        std::lock_guard<std::mutex> lock(fake.mutex);
        CHECK_RUNTIME(fake.interruptCalls == 1);
        CHECK_RUNTIME(fake.exitCalls == 1);
    }

    auto replacement = pool.acquire(error);
    CHECK_RUNTIME(replacement != nullptr);
    CHECK_RUNTIME(waitForEvents(fake, 2));
    {
        std::lock_guard<std::mutex> lock(fake.mutex);
        CHECK_RUNTIME(fake.initializeCalls == 2);
    }
    replacement.reset();
}

void concurrentAcquireTest() {
    using namespace usbhost::android;
    FakeLibusb fake;
    LibusbRuntimePool pool(hooks(fake));
    constexpr int threadCount = 8;
    std::atomic<bool> start{false};
    std::vector<std::shared_ptr<LibusbRuntime>> runtimes(threadCount);
    std::vector<std::thread> workers;
    for (int index = 0; index < threadCount; ++index) {
        workers.emplace_back([&, index] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            int error = 0;
            runtimes[index] = pool.acquire(error);
            CHECK_RUNTIME(error == 0);
        });
    }
    start.store(true, std::memory_order_release);
    for (auto &worker : workers) {
        worker.join();
    }
    CHECK_RUNTIME(runtimes[0] != nullptr);
    for (const auto &runtime : runtimes) {
        CHECK_RUNTIME(runtime == runtimes[0]);
    }
    CHECK_RUNTIME(waitForEvents(fake, 1));
    {
        std::lock_guard<std::mutex> lock(fake.mutex);
        CHECK_RUNTIME(fake.initializeCalls == 1);
    }
    runtimes.clear();
}

void failedInitializationTest() {
    using namespace usbhost::android;
    FakeLibusb fake;
    fake.initializeResult = -12;
    LibusbRuntimePool pool(hooks(fake));
    int error = 0;
    CHECK_RUNTIME(pool.acquire(error) == nullptr);
    CHECK_RUNTIME(error == -12);
    std::lock_guard<std::mutex> lock(fake.mutex);
    CHECK_RUNTIME(fake.initializeCalls == 1);
    CHECK_RUNTIME(fake.eventCalls == 0);
    CHECK_RUNTIME(fake.interruptCalls == 0);
    CHECK_RUNTIME(fake.exitCalls == 0);
}

}  // namespace

int runLibusbRuntimeContractTest() {
    failures.store(0, std::memory_order_relaxed);
    referenceCountingAndLifecycleTest();
    concurrentAcquireTest();
    failedInitializationTest();
    return failures.load(std::memory_order_relaxed);
}

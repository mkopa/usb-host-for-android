#include "android/libusb_runtime.hpp"

#include <chrono>
#include <new>
#include <system_error>

#if defined(ANDROID)
#include "libusb.h"
#endif

namespace usbhost::android {

namespace {

constexpr int kInvalidHooks = -2;
constexpr int kThreadStartFailure = -99;

#if defined(ANDROID)
int productionInitialize(void *, bool noDeviceDiscovery, LibusbContext *outContext) {
    if (!noDeviceDiscovery || outContext == nullptr) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }
    libusb_context *context = nullptr;
    libusb_init_option option{};
    option.option = LIBUSB_OPTION_NO_DEVICE_DISCOVERY;
    option.value.ival = 1;
    const int result = libusb_init_context(&context, &option, 1);
    *outContext = context;
    return result;
}

int productionHandleEvents(void *, LibusbContext context) {
    return libusb_handle_events(static_cast<libusb_context *>(context));
}

void productionInterruptEvents(void *, LibusbContext context) {
    libusb_interrupt_event_handler(static_cast<libusb_context *>(context));
}

void productionExitContext(void *, LibusbContext context) {
    libusb_exit(static_cast<libusb_context *>(context));
}
#else
int productionInitialize(void *, bool, LibusbContext *outContext) {
    if (outContext != nullptr) {
        *outContext = nullptr;
    }
    return -12;
}

int productionHandleEvents(void *, LibusbContext) {
    return -12;
}

void productionInterruptEvents(void *, LibusbContext) {}
void productionExitContext(void *, LibusbContext) {}
#endif

}  // namespace

bool LibusbRuntimeHooks::isValid() const noexcept {
    return initialize != nullptr && handleEvents != nullptr && interruptEvents != nullptr &&
        exitContext != nullptr;
}

LibusbRuntime::LibusbRuntime(LibusbRuntimeHooks hooks, LibusbContext context) noexcept
    : hooks_(hooks), context_(context) {}

LibusbRuntime::~LibusbRuntime() {
    stopping_.store(true, std::memory_order_release);
    if (context_ != nullptr) {
        hooks_.interruptEvents(hooks_.userData, context_);
    }
    if (eventThread_.joinable()) {
        eventThread_.join();
    }
    if (context_ != nullptr) {
        hooks_.exitContext(hooks_.userData, context_);
        context_ = nullptr;
    }
}

LibusbContext LibusbRuntime::nativeContext() const noexcept {
    return context_;
}

std::shared_ptr<LibusbRuntime> LibusbRuntime::create(const LibusbRuntimeHooks &hooks,
                                                     int &outError) {
    outError = 0;
    if (!hooks.isValid()) {
        outError = kInvalidHooks;
        return nullptr;
    }

    LibusbContext context = nullptr;
    outError = hooks.initialize(hooks.userData, true, &context);
    if (outError != 0 || context == nullptr) {
        if (outError == 0) {
            outError = kInvalidHooks;
        }
        return nullptr;
    }

    std::shared_ptr<LibusbRuntime> runtime;
    try {
        runtime.reset(new LibusbRuntime(hooks, context));
    } catch (const std::bad_alloc &) {
        hooks.exitContext(hooks.userData, context);
        outError = kThreadStartFailure;
        return nullptr;
    }
    if (!runtime->startEventThread(outError)) {
        runtime.reset();
        return nullptr;
    }
    return runtime;
}

bool LibusbRuntime::startEventThread(int &outError) noexcept {
    try {
        eventThread_ = std::thread(&LibusbRuntime::eventLoop, this);
        outError = 0;
        return true;
    } catch (const std::system_error &) {
        outError = kThreadStartFailure;
        return false;
    }
}

void LibusbRuntime::eventLoop() noexcept {
    while (!stopping_.load(std::memory_order_acquire)) {
        const int result = hooks_.handleEvents(hooks_.userData, context_);
        if (result != 0 && !stopping_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

LibusbRuntimePool::LibusbRuntimePool(LibusbRuntimeHooks hooks) noexcept : hooks_(hooks) {}

std::shared_ptr<LibusbRuntime> LibusbRuntimePool::acquire(int &outError) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto current = active_.lock()) {
        outError = 0;
        return current;
    }
    auto runtime = LibusbRuntime::create(hooks_, outError);
    if (runtime) {
        active_ = runtime;
    }
    return runtime;
}

LibusbRuntimeHooks productionLibusbRuntimeHooks() noexcept {
    return {nullptr, productionInitialize, productionHandleEvents,
            productionInterruptEvents, productionExitContext};
}

std::shared_ptr<LibusbRuntime> acquireLibusbRuntime(int &outError) {
    static LibusbRuntimePool pool(productionLibusbRuntimeHooks());
    return pool.acquire(outError);
}

}  // namespace usbhost::android

#ifndef USBHOST_ANDROID_LIBUSB_RUNTIME_HPP
#define USBHOST_ANDROID_LIBUSB_RUNTIME_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace usbhost::android {

using LibusbContext = void *;

struct LibusbRuntimeHooks {
    void *userData{nullptr};
    int (*initialize)(void *userData, bool noDeviceDiscovery,
                      LibusbContext *outContext){nullptr};
    int (*handleEvents)(void *userData, LibusbContext context){nullptr};
    void (*interruptEvents)(void *userData, LibusbContext context){nullptr};
    void (*exitContext)(void *userData, LibusbContext context){nullptr};

    bool isValid() const noexcept;
};

class LibusbRuntime final {
public:
    ~LibusbRuntime();
    LibusbRuntime(const LibusbRuntime &) = delete;
    LibusbRuntime &operator=(const LibusbRuntime &) = delete;

    LibusbContext nativeContext() const noexcept;

private:
    friend class LibusbRuntimePool;

    LibusbRuntime(LibusbRuntimeHooks hooks, LibusbContext context) noexcept;
    static std::shared_ptr<LibusbRuntime> create(const LibusbRuntimeHooks &hooks,
                                                 int &outError);
    bool startEventThread(int &outError) noexcept;
    void eventLoop() noexcept;

    LibusbRuntimeHooks hooks_;
    LibusbContext context_{nullptr};
    std::atomic<bool> stopping_{false};
    std::thread eventThread_;
};

/** Shares exactly one runtime while at least one acquired reference remains alive. */
class LibusbRuntimePool final {
public:
    explicit LibusbRuntimePool(LibusbRuntimeHooks hooks) noexcept;
    LibusbRuntimePool(const LibusbRuntimePool &) = delete;
    LibusbRuntimePool &operator=(const LibusbRuntimePool &) = delete;

    std::shared_ptr<LibusbRuntime> acquire(int &outError);

private:
    LibusbRuntimeHooks hooks_;
    std::mutex mutex_;
    std::weak_ptr<LibusbRuntime> active_;
};

LibusbRuntimeHooks productionLibusbRuntimeHooks() noexcept;
std::shared_ptr<LibusbRuntime> acquireLibusbRuntime(int &outError);

}  // namespace usbhost::android

#endif

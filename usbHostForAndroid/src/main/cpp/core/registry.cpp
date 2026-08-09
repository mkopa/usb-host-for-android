#include "core/registry.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace usbhost {

namespace {

std::mutex registryMutex;
std::unordered_map<usbhost_session, std::shared_ptr<Session>> sessions;
std::atomic<uint64_t> nextHandle{1};

}  // namespace

usbhost_session registerSession(std::shared_ptr<Session> session) {
    if (!session) {
        return 0;
    }
    usbhost_session handle = nextHandle.fetch_add(1, std::memory_order_relaxed);
    if (handle == 0) {
        handle = nextHandle.fetch_add(1, std::memory_order_relaxed);
    }
    std::lock_guard<std::mutex> lock(registryMutex);
    sessions.emplace(handle, std::move(session));
    return handle;
}

std::shared_ptr<Session> findSession(usbhost_session handle) {
    std::lock_guard<std::mutex> lock(registryMutex);
    const auto found = sessions.find(handle);
    return found == sessions.end() ? nullptr : found->second;
}

std::shared_ptr<Session> retireSession(usbhost_session handle) {
    std::lock_guard<std::mutex> lock(registryMutex);
    const auto found = sessions.find(handle);
    if (found == sessions.end()) {
        return nullptr;
    }
    std::shared_ptr<Session> session = found->second;
    sessions.erase(found);
    return session;
}

}  // namespace usbhost

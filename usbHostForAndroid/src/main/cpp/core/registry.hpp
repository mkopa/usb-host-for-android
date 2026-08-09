#ifndef USBHOST_CORE_REGISTRY_HPP
#define USBHOST_CORE_REGISTRY_HPP

#include <memory>

#include "core/session.hpp"

namespace usbhost {

usbhost_session registerSession(std::shared_ptr<Session> session);
std::shared_ptr<Session> findSession(usbhost_session handle);
std::shared_ptr<Session> retireSession(usbhost_session handle);

}  // namespace usbhost

#endif

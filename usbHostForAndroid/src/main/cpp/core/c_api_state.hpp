#ifndef USBHOST_CORE_C_API_STATE_HPP
#define USBHOST_CORE_C_API_STATE_HPP

#include <string_view>

#include "usbhost/usbhost.h"

namespace usbhost::detail {

usbhost_status setLastResult(usbhost_status status, std::string_view diagnostic);

}  // namespace usbhost::detail

#endif

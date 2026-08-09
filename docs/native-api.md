# Native API

The stable native contract is the C17-compatible header
`usbHostForAndroid/src/main/cpp/include/usbhost/usbhost.h`. The release AAR publishes it through a
Prefab package named `usbhost`.

## C++ consumption

Link the Prefab target from an Android CMake consumer and include the header:

```cmake
find_package(usbhost REQUIRED CONFIG)
target_link_libraries(your_native_target PRIVATE usbhost::usbhost)
```

```cpp
#include <usbhost/usbhost.h>
```

Android must grant permission and open `UsbDeviceConnection` before its file descriptor is passed to
`usbhost_open_stlink_v3_fd`. The library duplicates the descriptor; the caller retains ownership of
the original. All calls for one session are serialized. Close is idempotent.

## Rust preparation

The header is suitable for bindgen because it exposes only fixed-width integers, C enums, plain
size-prefixed structures, caller-owned buffers, and opaque numeric handles. A future Rust crate must:

- assert `usbhost_abi_version()` before opening a session;
- represent `usbhost_session` as an opaque newtype;
- keep the Android connection alive for the session lifetime;
- translate status codes without assuming diagnostic string stability;
- call `usbhost_close` from `Drop` while also exposing explicit close.

No Rust crate or JNI replacement is part of version 0.1.0.

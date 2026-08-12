# Native API

The stable native contract is the C17-compatible header
`usbHostForAndroid/src/main/cpp/include/usbhost/usbhost.h`. The release AAR publishes it through a
Prefab package named `usbhost`.

The umbrella header preserves the read-only STLINK API and includes the generic transport header
`usbhost/transport.h`. Both are stable C ABIs: incompatible changes require a new major version.

## C and C++ consumption

Link the Prefab target from an Android CMake consumer and include the header:

```cmake
find_package(usbhost REQUIRED CONFIG)
target_link_libraries(your_native_target PRIVATE usbhost::usbhost)
```

```cpp
#include <usbhost/usbhost.h>
```

Android must grant permission and open `UsbDeviceConnection` before its file descriptor is passed to
`usbhost_transport_open_fd` or `usbhost_open_stlink_v3_fd`. The descriptor is borrowed at the API
boundary and duplicated internally; the caller retains ownership of its Android connection. Calls
for one session are serialized, blocking calls require a worker thread, and close is idempotent.

Every size-versioned output structure must be zero-initialized and have `struct_size` set before a
call. Buffers remain caller-owned, and `out_actual_length` is meaningful for success and partial
terminal results.

```c
#include <usbhost/transport.h>

usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
usbhost_status status = usbhost_transport_open_fd(authorized_fd, &session);
if (status != USBHOST_OK) return status;

usbhost_transport_device_descriptor device = {0};
device.struct_size = sizeof(device);
status = usbhost_transport_get_device_descriptor(session, &device);
if (status == USBHOST_OK) {
    status = usbhost_transport_claim_interface(session, 0);
}

uint8_t buffer[64] = {0};
uint32_t actual = 0;
if (status == USBHOST_OK) {
    status = usbhost_transport_bulk_transfer(
        session, 0x81, buffer, sizeof(buffer), 3000, &actual);
}

(void)usbhost_transport_release_interface(session, 0);
(void)usbhost_transport_close(session);
```

The endpoint must belong to the active alternate setting of a claimed interface and match the
requested transfer type. Production adapters must discover it from the descriptor snapshot rather
than assuming the example address. C++ consumers use the same functions and may wrap the opaque
handle with RAII; they must not expose C++ exceptions across this boundary.

## ABI rules

- Status numbers, existing functions, and structure prefixes are stable within major version 1.
- Public structures use fixed-width fields and `struct_size`; future compatible fields are appended.
- Transfer buffers and descriptor destinations are allocated and retained by the caller.
- Diagnostic strings are sanitized, thread-local implementation details and are not stable keys.
- `usbhost_transport_close` cancels active work and completes cleanup within the documented bound.
- No libusb, JNI, Android framework, C++ STL, exception, template, or allocator-owned object crosses
  the C ABI.

## Rust preparation

The headers are suitable for bindgen because they expose only fixed-width integers, C enums, plain
size-prefixed structures, caller-owned buffers, and opaque numeric handles. A future Rust crate must:

- assert `usbhost_abi_version()` before opening a session;
- represent `usbhost_session` and `usbhost_transport_session` as separate opaque newtypes;
- keep the Android connection alive for the session lifetime;
- translate status codes without assuming diagnostic string stability;
- model mutable transfer buffers as exclusively borrowed for the complete blocking call;
- call the matching close function from `Drop` while also exposing explicit close;
- keep Android permission and file-descriptor acquisition outside the portable Rust wrapper.

Generated bindings should allowlist `usbhost_*`, use the published Prefab header from the AAR, and
pin the major ABI version. No Rust crate or JNI replacement is part of version 0.1.0.

See [the managed adapter example](transport-adapter-example.md) for lifecycle, endpoint selection,
and capability boundaries shared by Java, Kotlin, C++, and future Rust adapters.

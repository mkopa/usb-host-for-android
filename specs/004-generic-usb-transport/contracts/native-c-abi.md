# Native C ABI Contract

## Header and linkage

The additive public header is `usbhost/transport.h`, included by `usbhost/usbhost.h`. All functions
use C linkage and `USBHOST_API`. Public records use fixed-width integers, begin with `struct_size`,
and contain no pointer with library-owned lifetime.

## Stable handles and statuses

```c
typedef uint64_t usbhost_transport_session;
```

`0` is never a valid session. Existing `usbhost_status` numeric values remain unchanged. New values
for stall, cancelled, and unsupported operation append to the enum. C and managed tests assert the
mapping.

## Session functions

```c
usbhost_status usbhost_transport_open_fd(
    int authorized_fd,
    usbhost_transport_session *out_session);

usbhost_status usbhost_transport_cancel(
    usbhost_transport_session session);

usbhost_status usbhost_transport_close(
    usbhost_transport_session session);
```

Open duplicates `authorized_fd` and leaves caller ownership unchanged. Failed open publishes no
handle. Cancel is thread-safe and may race with completion. Close is idempotent for a retired handle
owned by the current process but rejects fabricated/stale generations.

## Descriptor query functions

The ABI provides size-versioned records for device, configuration, interface, alternate setting,
endpoint, and additional-descriptor metadata. Records carry the snapshot generation and raw USB
fields. Enumeration uses stable zero-based indices within one snapshot generation:

```c
usbhost_status usbhost_transport_get_device_descriptor(
    usbhost_transport_session session,
    usbhost_transport_device_descriptor *out_descriptor);

usbhost_status usbhost_transport_get_configuration_count(
    usbhost_transport_session session,
    uint32_t *out_count);

usbhost_status usbhost_transport_get_configuration_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    usbhost_transport_configuration_descriptor *out_descriptor);

usbhost_status usbhost_transport_get_interface_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t *out_count);

usbhost_status usbhost_transport_get_interface_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    usbhost_transport_interface_descriptor *out_descriptor);

usbhost_status usbhost_transport_get_alternate_setting_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t *out_count);

usbhost_status usbhost_transport_get_alternate_setting_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    usbhost_transport_alternate_setting_descriptor *out_descriptor);

usbhost_status usbhost_transport_get_endpoint_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    uint32_t *out_count);

usbhost_status usbhost_transport_get_endpoint_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    uint32_t endpoint_index,
    usbhost_transport_endpoint_descriptor *out_descriptor);

usbhost_status usbhost_transport_get_additional_descriptor_at(
    usbhost_transport_session session,
    const usbhost_transport_descriptor_location *location,
    uint8_t *destination,
    uint32_t capacity,
    uint8_t *out_descriptor_type,
    uint32_t *out_actual_length);
```

Every function receives an initialized output record with `struct_size`. Additional descriptor bytes
are copied into a caller buffer with capacity and returned required/actual length. No function
returns a libusb pointer or library allocation. `usbhost_transport_descriptor_location` contains a
size, generation, scope enum, zero-based configuration/interface/alternate/endpoint indices, and
additional-descriptor index; indices outside the selected scope must be zero.

## Configuration and interface functions

```c
usbhost_status usbhost_transport_select_configuration(
    usbhost_transport_session session,
    uint8_t configuration_value);

usbhost_status usbhost_transport_claim_interface(
    usbhost_transport_session session,
    uint8_t interface_number);

usbhost_status usbhost_transport_select_alternate_setting(
    usbhost_transport_session session,
    uint8_t interface_number,
    uint8_t alternate_setting);

usbhost_status usbhost_transport_release_interface(
    usbhost_transport_session session,
    uint8_t interface_number);
```

Configuration selection fails with `BUSY` while any interface is claimed and never changes state
implicitly. Claim/release and alternate selection are serialized per session.

## Transfer functions

```c
usbhost_status usbhost_transport_control_transfer(
    usbhost_transport_session session,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);

usbhost_status usbhost_transport_bulk_transfer(
    usbhost_transport_session session,
    uint8_t endpoint_address,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);

usbhost_status usbhost_transport_interrupt_transfer(
    usbhost_transport_session session,
    uint8_t endpoint_address,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);
```

`buffer` may be null only for zero length. Control length is at most 65,535; bulk/interrupt length is
at most 1 MiB; timeout is 1–60,000 ms. `out_actual_length` is written for completed, partial,
cancelled, and timed-out operations when the backend supplies a count. Short success is `OK`.

## Diagnostics and symbol policy

Existing thread-local `usbhost_last_status()` and `usbhost_last_error()` remain available and
sanitized. Export inspection permits only the documented C ABI, required JNI entry points, and the
existing stable symbols; libusb and C++ symbols remain hidden.

## Compatibility verification

- Compile C11/C17 and exception-free C++17 consumers from published Prefab headers.
- Assert struct prefix offsets/sizes and existing enum numbers.
- Compile the previous same-major baseline against the candidate shared library.
- Reject removed/renamed exports, semantic changes, allocator-owned return buffers, and Android/JNI
  types in public C headers.

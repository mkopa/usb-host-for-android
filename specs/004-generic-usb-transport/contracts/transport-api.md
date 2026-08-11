# Generic USB Transport Contract

## Capability boundary

The transport supplies Android-authorized descriptor access, explicit configuration/interface state,
and control/bulk/interrupt transfers. It does not grant USB permission, scan devices, expose libusb
types, run arbitrary desktop binaries, execute isochronous transfers, or claim that a device class is
supported without a verified adapter.

## Ownership

- The application owns `UsbDevice` and `UsbDeviceConnection` before, during, and after generic open.
- Native open duplicates the connection FD. The transport owns and closes only the duplicate.
- libusb wraps but does not close that duplicate; transport close calls `libusb_close()` first.
- A `GenericUsbDevice` owns one opaque native session and immutable descriptor snapshots.
- A claimed `GenericUsbInterface` belongs to exactly one parent session and releases at most once.
- Transfer buffers remain caller-owned and native code retains no address after completion.
- Existing `StlinkSession` connection ownership remains unchanged for API compatibility.

## Threading and cancellation

- Public calls are synchronous and reject Android main-thread execution when they may block.
- One transfer at a time is permitted per session; different sessions may progress concurrently.
- Android I/O is internally asynchronous and completed by the shared libusb event runtime.
- `close()` marks closing, rejects new work, requests cancellation, observes the completion callback,
  releases resources within 2 seconds, and is idempotent.
- Cancellation may report a non-zero partial count; callers cannot assume no bytes moved.

## Descriptors and configuration

- All public descriptor objects are immutable copies and contain no libusb pointer.
- Standard device/configuration/interface/alternate-setting/endpoint fields are preserved.
- Additional descriptors are immutable, bounded `type + raw bytes` records at the scope reported by
  libusb. Malformed or overflowing descriptors fail the snapshot atomically.
- Configuration selection is explicit, never automatic, and allowed only with zero claimed
  interfaces. Success refreshes active settings/endpoints and advances the snapshot generation.
- Alternate-setting selection requires a claimed interface and invalidates prior endpoint objects.

## Transfer limits

| Transfer | Length | Timeout |
|---|---:|---:|
| Control | 0–65,535 bytes | 1–60,000 ms |
| Bulk | 0–1,048,576 bytes | 1–60,000 ms |
| Interrupt | 0–1,048,576 bytes | 1–60,000 ms |

Offsets, sums, narrowing conversions, endpoint membership/direction/type, and descriptor generation
are validated before JNI pin/copy or backend submission. Endpoint zero is control-only. Isochronous
endpoint metadata is inspectable, while execution returns unsupported operation.

## Result and error behavior

- Success returns the actual count; a short packet is successful.
- IN transfers modify only the actual-count portion of the caller's requested slice.
- Stable statuses cover invalid argument/state, permission, unsupported device/operation, busy,
  timeout, stall, cancellation, disconnect, USB failure, and internal failure.
- Diagnostics are bounded and exclude serial number, USB filesystem path, raw descriptor dump, local
  device identifier, pointer value, and private application data.

## Compatibility

- Managed API and C ABI are stable from first public transport release.
- Same-major changes are additive and backward compatible.
- Existing status numeric values, function semantics, symbols, and struct prefixes never change.
- Public structs start with `struct_size`; new fields append and unknown trailing fields are ignored.
- Incompatible changes require a new major version and migration guidance.
- JNI names and libusb structures are private implementation details.

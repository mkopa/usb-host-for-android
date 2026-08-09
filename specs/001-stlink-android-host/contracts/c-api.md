# C API Contract

The authoritative declaration is `usbHostForAndroid/src/main/cpp/include/usbhost/usbhost.h`.

## ABI rules

- Functions use `extern "C"` and fixed-width integer types.
- A session is an opaque non-zero `uint64_t`; callers never cast it to a pointer.
- Structures start with `uint32_t struct_size`. Callers initialize it to `sizeof(struct)`.
- Existing status numbers and field offsets are never changed within a major version.
- Functions return `usbhost_status`; no C++ exception crosses the boundary.
- `usbhost_last_error()` is thread-local, UTF-8, and valid until the next API call on that thread.
- Input file descriptors are duplicated. The caller retains ownership of the supplied descriptor;
  the library owns and closes only its duplicate.
- Read buffers are caller-owned and must remain writable for the duration of the call.

## Status values

| Name | Value | Meaning |
|------|-------|---------|
| `USBHOST_OK` | 0 | Operation completed |
| `USBHOST_INVALID_ARGUMENT` | 1 | Null, invalid size, range, speed, or descriptor |
| `USBHOST_PERMISSION_DENIED` | 2 | Descriptor cannot be used |
| `USBHOST_UNSUPPORTED_DEVICE` | 3 | USB VID/PID or mode unsupported |
| `USBHOST_USB_ERROR` | 4 | Non-terminal USB transport failure |
| `USBHOST_TIMEOUT` | 5 | Operation timed out |
| `USBHOST_DISCONNECTED` | 6 | Programmer detached; session terminal |
| `USBHOST_PROGRAMMER_ERROR` | 7 | ST-Link command/version failure |
| `USBHOST_TARGET_NOT_FOUND` | 8 | No responding target |
| `USBHOST_UNSUPPORTED_TARGET` | 9 | Target is not STM32G0B/G0C ID `0x467` |
| `USBHOST_INVALID_STATE` | 10 | Operation not allowed in current lifecycle state |
| `USBHOST_BUSY` | 11 | Operation rejected due to concurrent use |
| `USBHOST_INTERNAL_ERROR` | 12 | Allocation, invariant, or unexpected native failure |

## Operations

```c
usbhost_status usbhost_open_stlink_v3_fd(
    int fd,
    uint16_t vendor_id,
    uint16_t product_id,
    uint32_t swd_frequency_khz,
    usbhost_session *out_session,
    usbhost_programmer_info *out_programmer);

usbhost_status usbhost_connect_target(
    usbhost_session session,
    usbhost_target_info *out_target);

usbhost_status usbhost_read_memory(
    usbhost_session session,
    uint32_t address,
    uint8_t *destination,
    uint32_t length);

usbhost_status usbhost_close(usbhost_session session);

const char *usbhost_status_name(usbhost_status status);
usbhost_status usbhost_last_status(void);
const char *usbhost_last_error(void);
```

## Behavioral guarantees

- Open validates ST-Link V3 debug VID/PID, duplicates the descriptor, wraps it through libusb, claims
  the debug interface, reads programmer version, and does not require a connected target.
- Target connection uses hot-plug semantics and does not intentionally reset, halt, erase, or write
  the target. It succeeds only for chip ID `0x467`.
- Reads accept flash or SRAM ranges only, at most one MiB per call, with overflow checked before I/O.
- Calls on one session are serialized. Close waits for the active call and returns success when
  repeated, including for an already-retired or otherwise unknown handle. Non-close operations on an
  unknown or retired handle return `USBHOST_INVALID_STATE` without dereferencing.

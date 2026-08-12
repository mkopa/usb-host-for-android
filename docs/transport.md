# Generic USB Transport

`info.marcin.usbhost.transport` is a stable adapter foundation for Android API 23+. It converts an
Android-authorized `UsbDeviceConnection` into immutable USB descriptor snapshots and synchronous
control, bulk, and interrupt transfers. It does not replace Android USB Host permission handling or
implement a device-class protocol.

## Setup

Add the AAR dependency from Maven Central after release, declare `android.hardware.usb.host` in the
application manifest, discover devices with `UsbManager`, request Android permission, and open a
`UsbDeviceConnection`. Call `GenericUsbDevice.open(device, connection)` on a worker thread only.

```kotlin
withContext(Dispatchers.IO) {
    GenericUsbDevice.open(device, connection).use { usb ->
        val descriptor = usb.descriptor
        val configurations = usb.configurations
    }
}
connection.close() // still owned by the application
```

The application owns the `UsbDevice`, permission lifecycle, and `UsbDeviceConnection` throughout.
Native open duplicates the authorized file descriptor and closes only its duplicate. The connection
must remain alive until library work completes, and the application closes it explicitly.

## Lifecycle and threading

- `open`, configuration/alternate selection, claim/release, transfers, cancellation, and `close`
  are blocking and reject Android's main Looper.
- Descriptor and state getters are immutable, non-blocking snapshots.
- One transfer at a time is allowed per session. Independent sessions may run concurrently.
- A claimed `GenericUsbInterface` belongs to one parent and releases at most once.
- `close` rejects new work, cancels active work, releases claims and the duplicated FD within two
  seconds, and is safe to repeat.
- Cancellation and terminal errors may report a non-zero partial byte count.

Use an executor, coroutine `Dispatchers.IO`, or another application-controlled worker. A coordinator
may call `cancelActiveTransfer` or `close` from another worker while a transfer is blocked.

## Descriptors, configurations, and endpoints

Descriptor values are copies, not libusb pointers. Standard device, configuration, interface,
alternate-setting, and endpoint fields are available together with bounded immutable additional
descriptor records (`type + raw bytes`).

Configuration selection is explicit and allowed only while no interface is claimed. Claim the
interface before bulk or interrupt I/O. Alternate-setting selection requires that claim. Successful
configuration or alternate selection refreshes the snapshot; previously captured endpoint objects
become stale and are rejected before native submission.

Never choose an interface or endpoint solely by its numeric position. A protocol adapter must match
the documented class/subclass/protocol and endpoint direction/type/address for hardware it has
validated.

## Limits

| Operation | Data length | Timeout |
|---|---:|---:|
| Control | 0–65,535 bytes | 1–60,000 ms |
| Bulk | 0–1,048,576 bytes | 1–60,000 ms |
| Interrupt | 0–1,048,576 bytes | 1–60,000 ms |

Offsets, sums, unsigned USB fields, endpoint membership/direction/type, and snapshot generation are
validated before JNI buffer access. Endpoint zero is control-only. Isochronous descriptors may be
inspected, but isochronous execution is unsupported.

## Results and errors

`UsbTransferResult` reports stable status plus actual byte count. A short packet with `OK` is normal.
`UsbTransportException` reports a stable `UsbTransportStatus`, sanitized diagnostic, and partial
actual length when applicable. Applications should branch on the status, not message text.

Important terminal states include `TIMEOUT`, `STALL`, `CANCELLED`, and `DISCONNECTED`. Recover by
ending the affected operation/session according to the adapter protocol; never retry a partially
completed OUT command unless that protocol defines the retry as safe.

## Capability boundary

The transport does not grant USB permission, scan `/dev/bus/usb`, expose libusb objects, load desktop
binaries, or support isochronous transfers. Generic transport success does not establish support for
serial, DFU, CMSIS-DAP, HID, printers, analyzers, programmers, or any other class. Such support
requires a separate adapter, deterministic tests, and a dated sanitized hardware record.

STLINK-V3 is the first existing product adapter and retains its read-only public operations. See
[the adapter example](transport-adapter-example.md), [native API](native-api.md), and
[hardware validation policy](hardware/README.md).

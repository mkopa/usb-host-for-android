# Hardware validation status

Physical read-only validation is recorded for Samsung Galaxy A54, ST-Link V3 `0483:3754`, and
STM32G0B0RET6 in
[`2026-08-08-samsung-a54-stlink-v3-stm32g0b0.md`](2026-08-08-samsung-a54-stlink-v3-stm32g0b0.md).

Device-specific validation of the `RtlSdrForAndroid` example on a generic `0BDA:2838` RTL2832U
dongle with an R820T/R820T2 tuner is recorded in
[`2026-08-12-samsung-a54-rtlsdr-r820t.md`](2026-08-12-samsung-a54-rtlsdr-r820t.md). That session
writes volatile tuner registers and therefore sits outside the generic smoke procedure below; it
establishes example behavior only and does not extend the generic transport support record.

Unchecked scenarios in those records remain required before claiming complete transport recovery and
permission-denial coverage on physical hardware. Automated host, JVM, and Android cross-compilation
tests complement but do not replace the dated hardware evidence.

## Generic transport smoke procedure

Run this optional procedure only on a public demonstration device whose inspected interfaces and
requests are known to be non-destructive. A generic transport result validates lifecycle and I/O
primitives; it does not establish support for the device class or protocol.

1. Record the library commit, application version, Android model/API/ABI, public USB VID:PID, and a
   generic public description of the demonstration device. Do not record serial numbers.
2. Discover the device with Android `UsbManager`, exercise both permission denial and permission
   grant, and open the application-owned `UsbDeviceConnection`.
3. On a worker thread, open `GenericUsbDevice`. Verify vendor/product IDs match Android and record
   counts only for configurations, interfaces, alternate settings, endpoints, and additional
   descriptors. Do not save raw descriptor bytes if they contain identifying data.
4. If the public device profile identifies a safe interface, claim and release it. Select a
   configuration or alternate setting only when the profile declares the transition safe.
5. Perform only an explicitly documented non-mutating control, bulk, or interrupt IN transaction.
   Record status, requested/actual counts, and timing; never record payload bytes.
6. Verify closing the generic session does not close the application connection, then close the
   application connection explicitly.
7. In a separate run, disconnect during safe inspection/IN transfer. Verify terminal status and
   bounded close (at most two seconds), then reconnect and repeat a normal open/close.
8. Copy [the evidence template](evidence-template.md), remove unused fields, audit it for identifiers
   and local paths, and commit it only when every claimed scenario has an observed result.

Do not issue OUT/vendor requests, reset, firmware update, erase, program, register write, target
write, or undocumented class commands. Hashes of payloads, USB serials, raw memory, screenshots with
identifiers, full `adb` device lists, and local filesystem paths are not acceptable public evidence.

## Evidence interpretation

- Automated fake-transport tests prove deterministic contracts, not hardware compatibility.
- A generic smoke PASS proves only the recorded transport operations on that device/revision.
- A protocol/class support claim requires a separate implemented adapter and its own evidence.
- A failed or incomplete row remains visible as FAIL/NOT RUN; it must not be rewritten as supported.

# Hardware Evidence

**Date**: YYYY-MM-DD
**Result**: PASS / FAIL
**Library revision**: commit
**Validation type**: Generic transport / STLINK-V3 read-only

## Environment

- Android device model:
- Android version / API:
- CPU ABI:
- Test application version:
- USB VID:PID (public identifiers only):
- Demonstration device description:
- Interface/endpoint profile source:
- ST-Link V3 model:
- ST-Link firmware (V/J/S):
- Target board / MCU marking:
- Target power and measured VTref:
- SWD wiring and cable length:
- SWD frequency:

Use `Not applicable` for STLINK-only fields during generic transport validation. Do not record USB
serial numbers, Android device identifiers, raw descriptors/payloads/target memory, screenshots with
identifiers, local paths, private application names, or customer/employer information.

## Generic transport scenarios

| Scenario | PASS / FAIL / NOT RUN | Sanitized observation |
|---|---|---|
| Android discovery and permission denial | | |
| Permission grant and caller-owned connection open | | |
| Worker-thread generic session open | | |
| VID:PID matches Android device | | |
| Descriptor hierarchy counts inspected | | |
| Documented-safe interface claim/release | | |
| Documented-safe non-mutating IN transfer | | requested/actual count and timing only |
| Session close leaves caller connection usable | | |
| Disconnect returns terminal status | | no payload or device path |
| Close completes within two seconds | | elapsed time only |
| Reconnect and clean open/close | | |

No generic result in this section establishes protocol or device-class support. Write `NOT RUN` for
operations lacking a public non-destructive profile; do not improvise vendor/class commands.

## STLINK-V3 read-only scenarios

- [ ] Android discovers exactly the expected supported programmer.
- [ ] Permission denial returns `PERMISSION_DENIED` and leaves no session.
- [ ] Programmer opens without a connected target.
- [ ] Target identification reports chip ID `0x467` twenty consecutive times.
- [ ] Flash reports 512 KiB, SRAM 144 KiB, and page size 2 KiB.
- [ ] Reads of 1 B, 4 B, 1 KiB, and 64 KiB match an independent SHA-256 twenty times each.
- [ ] Invalid and overflowing ranges perform no USB read.
- [ ] Detach during read returns `DISCONNECTED`; repeated close is safe.

## Observations

Record timings, redacted diagnostics, unexpected behavior, and recovery steps here.

## Sanitization audit

- [ ] No USB serial number, Android identifier, IP/MAC, or raw payload/memory appears.
- [ ] No local filesystem path, private brand, application, customer, or employer name appears.
- [ ] Device description and VID:PID are public/generic.
- [ ] Every support statement maps to a PASS row; incomplete rows remain NOT RUN.
- [ ] The record contains no write, erase, reset, update, or undocumented request evidence.

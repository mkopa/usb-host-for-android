# ST-Link V3 / STM32G0B0RET6 Hardware Evidence

**Date**: 2026-08-08
**Result**: PASS for the scenarios marked below
**Library revision**: pre-release `001-stlink-android-host` validation worktree; the exact validated
artifact is identified by the APK SHA-256 below
**APK SHA-256**: `7350B63D7E633F094C2AE834754F78E2145F90402A5BF82AFDAAEE33C33384AB`

## Environment

- Android device model: Samsung SM-A546B (Galaxy A54)
- Android version / API: Android 16 / API 36
- CPU ABI: `arm64-v8a`
- ST-Link V3 model: USB `0483:3754`, product `STLINK-V3`
- ST-Link firmware (V/J/S): V3 / J17 / S not reported
- Target board / MCU marking: project MK3 / STM32G0B0RET6 (chip ID `0x467`)
- Target power and measured VTref: 3270 mV reported by ST-Link
- SWD wiring and cable length: connected by operator; not recorded
- SWD frequency: 1800 kHz

No USB serial number or raw target-memory content was recorded. The validation application
computed SHA-256 values only in volatile application memory and displayed only aggregate results.

## Scenarios

- [x] Android discovers exactly the expected supported programmer.
- [ ] Permission denial returns `PERMISSION_DENIED` and leaves no session (automated test only).
- [ ] Programmer opens without a connected target (not exercised in this hardware session).
- [ ] Target identification reports chip ID `0x467` twenty consecutive times (one hardware
  identification; repeated identity is covered by automated tests).
- [x] Flash reports 512 KiB, SRAM 144 KiB, and page size 2 KiB.
- [x] Reads of 1 B, 4 B, 1 KiB, and 64 KiB produced stable SHA-256 values twenty times each.
- [x] Invalid and overflowing ranges perform no USB read (automated native test).
- [ ] Detach during read returns `DISCONNECTED`; repeated close is safe (repeated close is covered
  by automated tests; physical detach was not exercised).

## Observations

- Android enumerated the ST-Link as `/dev/bus/usb/001/006` during the session.
- The read-only validation completed 80 flash reads with no mismatch.
- The native transport's read-only guard denied target register writes, memory writes, reset,
  run, step, and force-debug operations for the entire hardware session.
- Closing the session used the ST-Link transport detach command directly and did not use the
  upstream helper that writes target debug registers.
- No erase, program, option-byte, target-memory write, halt, run, or reset operation was requested.

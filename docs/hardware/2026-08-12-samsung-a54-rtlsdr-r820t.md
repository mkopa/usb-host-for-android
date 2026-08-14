# RTL-SDR / R820T2 Hardware Evidence

**Date**: 2026-08-12
**Result**: PASS for the scenarios marked below
**Library revision**: `181fa8b` (`feat/rtlsdr-for-android-example`)
**Validation type**: RtlSdrForAndroid example, device-specific

**Provenance**: This record transcribes the maintainer attestation published in
`rtlSdrForAndroid/README.md`. Rows marked NOT RUN were not exercised or not recorded during that
session. No row may be promoted to PASS without a dated, observed result.

## Environment

- Android device model: Samsung SM-A546B (Galaxy A54)
- Android version / API: Android 16 / API 36
- CPU ABI: `arm64-v8a`
- Test application version: local debug build of `rtlSdrForAndroid` at revision `181fa8b`
- USB VID:PID (public identifiers only): `0BDA:2838`
- Demonstration device description: generic RTL2832U USB dongle with an R820T/R820T2 tuner
- Interface/endpoint profile source: official rtl-sdr `v2.0.3` R82xx driver and `rtl_fm -M wbfm`
  pipeline
- ST-Link V3 model: Not applicable
- ST-Link firmware (V/J/S): Not applicable
- Target board / MCU marking: Not applicable
- Target power and measured VTref: Not applicable
- SWD wiring and cable length: Not applicable
- SWD frequency: Not applicable

No USB serial number, Android device identifier, raw descriptor dump, or IQ/PCM payload was
recorded.

## Scope deviation from the generic smoke procedure

The generic transport smoke procedure in [README.md](README.md) permits only non-mutating IN
transactions. This validation is deliberately outside that procedure: the FM player issues vendor
control transfers that write **volatile tuner and demodulator registers**, as required by the
official `rtl_fm` pipeline. No EEPROM write, firmware update, erase, or other persistent mutation
was performed, and device state is lost on power cycle. This record therefore establishes RTL-SDR
example behavior only; it does not extend the generic transport support record.

## Generic transport scenarios

| Scenario | PASS / FAIL / NOT RUN | Sanitized observation |
|---|---|---|
| Android discovery and permission denial | NOT RUN | denial path not exercised in this session |
| Permission grant and caller-owned connection open | PASS | `UsbManager` grant, application-owned connection |
| Worker-thread generic session open | PASS | opened through `GenericUsbDevice` off the main thread |
| VID:PID matches Android device | PASS | `0BDA:2838` |
| Descriptor hierarchy counts inspected | PASS | interface 0 descriptor and endpoints displayed |
| Documented-safe interface claim/release | PASS | interface 0 claimed and released |
| Documented-safe non-mutating IN transfer | PASS | bulk IN stream on endpoint `0x81`; counts not recorded |
| Session close leaves caller connection usable | NOT RUN | not separately observed |
| Disconnect returns terminal status | NOT RUN | physical detach not exercised |
| Close completes within two seconds | NOT RUN | elapsed time not measured |
| Reconnect and clean open/close | NOT RUN | not exercised |

No generic result in this section establishes protocol or device-class support.

## STLINK-V3 read-only scenarios

Not applicable.

## Observations

- Tuned 93.9 MHz WBFM and produced clear, continuous audio through Android `AudioTrack` for the
  duration of the session.
- I/Q demodulation ran entirely in the native `rtl_fm` port; Kotlin processed no I/Q samples.
- Volatile tuner register configuration succeeded; no EEPROM write was requested or performed.
- Session duration, transfer counts, timing, and error-path behavior were not recorded.

## Sanitization audit

- [x] No USB serial number, Android identifier, IP/MAC, or raw payload/memory appears.
- [x] No local filesystem path, private brand, application, customer, or employer name appears.
- [x] Device description and VID:PID are public/generic.
- [x] Every support statement maps to a PASS row; incomplete rows remain NOT RUN.
- [x] The record contains no write, erase, reset, update, or undocumented request evidence beyond
      the volatile tuner configuration declared in the scope deviation above.

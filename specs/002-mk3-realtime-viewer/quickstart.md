# Quickstart: Validate MK3 Observation

## Prerequisites

- Android API 23+ device with USB OTG
- STLINK-V3 in a supported debug mode
- Powered MK3 with STM32G0B0RET6
- USB permission granted by the sample application
- Initialized `libusb` and `stlink` submodules

## Build and deterministic validation

Use the build and validation commands from the repository [README](../../README.md). Expected
results are a publishable AAR/Prefab package, passing portable contract tests, and no requirement for
physical USB during deterministic tests.

## Hardware observation scenario

1. Install the example application and explicitly grant Android USB permission.
2. Select the STLINK-V3; never auto-select when more than one supported probe is attached.
3. Confirm programmer firmware, target voltage, chip ID `0x467`, and MK3 firmware identity.
4. Start observation and wait for a coherent snapshot with valid generation and CRC.
5. Confirm the UI reports freshness and effective rate and marks detachment stale within one second.
6. Run for 30 minutes and record a redacted evidence document under `docs/hardware/`.
7. Independently confirm that no write, erase, reset, halt, run, or step request occurred.

## Platform status rule

Only Android USB OTG currently has recorded physical evidence. Desktop hosts and Rust bindings must
remain labeled experimental/in validation until their own contracts, recovery behavior, and evidence
are complete.

# Implementation Plan: STM32G0 Realtime Viewer Transport

**Branch**: `001-stlink-android-host` | **Date**: 2026-08-08 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/002-stm32g0-observer/spec.md`

## Summary

Extend the Android-first STLINK-V3 USB Host SDK with a read-only observation layer for coherent,
versioned demonstration-board snapshots. Android owns USB permission and lifecycle, the native C++
core owns validation and sampling state, and a stable C ABI remains the integration point for C++
and future Rust consumers. Android is the only hardware-validated platform; other hosts remain
experimental until separately recorded evidence exists.

## Technical Context

**Language/Version**: Java 17, C17 public ABI, C++17 portable core

**Primary Dependencies**: Android USB Host API, Android NDK r28.2.13676358, CMake 3.22.1,
libusb v1.0.30, pinned stlink revision `6a1d36530b7ec0004498fb80c947c46e6c537134`

**Storage**: Bounded in-memory sample history; optional redacted Markdown diagnostic evidence

**Testing**: Native fake-transport contract tests, JVM adapter tests, recorded Android hardware
validation; no new tests are executed as part of this documentation/SDK-polish increment

**Target Platform**: Supported: Android API 23+ over USB OTG; experimental/in validation: other
native hosts and future Rust FFI consumers

**Project Type**: Publishable Android AAR/Prefab SDK with portable native core and example app

**Performance Goals**: First coherent view within 3 s; default dashboard at 5 Hz; bounded adaptive
polling that accounts for every stale, skipped, or rejected sample

**Constraints**: Read-only public API; no erase/program/reset/halt/run/step; no Android main-thread
blocking; explicit USB permission; one serialized operation per session; redacted diagnostics

**Scale/Scope**: STLINK-V3 debug-mode PIDs, STM32G0B0RET6, 44 peripherals/625 registers/3885 fields,
one selected observation target per session

## Constitution Check

*GATE: Passed before research and passed again after Phase 1 design.*

- **Portable core, thin adapters**: PASS. Snapshot validation, history, and decoding stay native and
  Android-free; USB permission, descriptors, lifecycle, and JNI stay in Android adapters.
- **Explicit stable boundaries**: PASS. The observation contract uses ABI versioning, fixed-width
  structures, caller-owned buffers, opaque sessions, and documented cancellation/lifetime rules.
- **Hardware safety**: PASS. Observation is snapshot-first and read-only. Unsafe direct registers
  are rejected before transport access; Android claims require dated hardware evidence.
- **Minimal delivery**: PASS. This plan adds only the transport-facing observation contract and
  data model required by the existing viewer specification.
- **Dependency governance**: PASS. libusb and stlink are pinned submodules with provenance and
  license notices; no new dependency is introduced.

## Project Structure

### Documentation

```text
specs/002-stm32g0-observer/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
└── contracts/
    ├── observation-c-api.md
    └── snapshot-schema.md
```

### Source Code

```text
usbHostForAndroid/src/main/
├── java/dev/usbhost/android/       # Android USB permission/lifecycle and Java API
└── cpp/
    ├── include/usbhost/usbhost.h   # Stable C ABI / Prefab public header
    ├── core/                       # Portable sessions, validation, state
    ├── observer/                   # Planned snapshot reader/history/decoder boundary
    ├── stlink/                     # Android STLINK-V3 + libusb adapter
    └── jni/                        # Thin Java/native bridge

usbHostExample/                     # Minimal integration and permission flow
native-tests/                       # Portable fake-transport validation
docs/hardware/                      # Redacted, dated hardware evidence
```

**Structure Decision**: Keep one publishable SDK module. The viewer-facing observation layer belongs
in the portable native core; UI code and future Rust bindings consume the C ABI and do not own USB
protocol behavior.

## Complexity Tracking

No constitution violations require justification.

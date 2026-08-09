# Implementation Plan: ST-Link Android Host

**Branch**: `001-stlink-android-host` | **Date**: 2026-08-08 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/001-stlink-android-host/spec.md`

## Summary

Build an Android library that discovers ST-Link V3 devices through Android USB Host, passes an
authorized device file descriptor into a native C ABI, adapts libusb to that descriptor without
root/device enumeration, and reuses the stlink communication library for non-destructive SWD target
identification and bounded memory reads from STM32G0B0RET6. A Java API owns Android lifecycle and
JNI; a platform-neutral C++ session core owns state and validation; the C ABI is the stable boundary
for current C++ and future Rust callers.

## Technical Context

**Language/Version**: Java 17 source compatibility, C17, C++17

**Primary Dependencies**: Android USB Host API; Android NDK r28.2.13676358; CMake 3.22.1;
libusb v1.0.30 (`87a55632db62c9bdc58cd31d3ccfa673f1bb017f`); stlink develop pinned at
`6a1d36530b7ec0004498fb80c947c46e6c537134`

**Storage**: None; optional hardware evidence is stored as Markdown under `docs/hardware/`

**Testing**: JUnit 4 JVM tests, native CTest executable with a fake transport, Android instrumented
smoke tests where a device is available, recorded physical-hardware quickstart

**Target Platform**: Android API 23+, compile/target SDK 35; `arm64-v8a`, `armeabi-v7a`, and
`x86_64` AAR variants

**Project Type**: Android library with native shared library and a minimal Java sample application

**Performance Goals**: Discovery below 100 ms excluding platform enumeration; target open and
identity below 10 s; 64 KiB read below 10 s on validated hardware

**Constraints**: No root; no direct `/dev/bus/usb` discovery; no Android main-thread blocking;
read-only public v1 API; one operation per session; one MiB maximum read; deterministic close

**Scale/Scope**: ST-Link V3 debug PIDs and STM32G0B0RET6 (device ID `0x467`) only; one active
operation per session; no guarantee for concurrent sessions

## Constitution Check

*GATE: Passed before research and passed again after design.*

- **Portable core**: PASS. Session state, validation, error mapping, and C ABI contain no Android
  framework types. Android USB and JNI remain adapters.
- **Explicit boundaries**: PASS. The C contract uses fixed-width values, versioned structures,
  opaque handles, caller-owned buffers, and explicit file-descriptor duplication/ownership.
- **Hardware-safe testing**: PASS. Public v1 operations are read-only; pure behavior uses fakes;
  hardware claims require an evidence record and disconnect/timeout coverage.
- **Minimal delivery**: PASS. The feature supports one programmer generation, one MCU family, and
  discovery/open/identify/read/close only. Flashing and reset control are excluded.
- **Dependency governance**: PASS. Immutable dependency revisions, licenses, provenance, and
  Android build integration are documented in `THIRD_PARTY_NOTICES.md` and `research.md`.

## Project Structure

### Documentation (this feature)

```text
specs/001-stlink-android-host/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── c-api.md
│   └── java-api.md
└── tasks.md
```

### Source Code (repository root)

```text
usbHostForAndroid/
├── build.gradle
├── consumer-rules.pro
└── src/
    ├── main/
    │   ├── AndroidManifest.xml
    │   ├── java/dev/usbhost/android/
    │   └── cpp/
    │       ├── CMakeLists.txt
    │       ├── include/usbhost/usbhost.h
    │       ├── core/
    │       ├── jni/
    │       └── stlink/
    └── test/java/dev/usbhost/android/

usbHostExample/
└── src/main/
    ├── AndroidManifest.xml
    └── java/dev/usbhost/example/MainActivity.java

third_party/
├── libusb/                         # git submodule, v1.0.30
└── stlink/                         # git submodule, pinned develop revision

native-tests/
└── CMakeLists.txt

docs/hardware/
└── evidence-template.md
```

**Structure Decision**: A single publishable Android library contains the Java adapter, JNI, stable
C ABI, portable C++ core, and native transports. A separate minimal application proves Java usage.
Third-party projects remain immutable submodules; Android-specific integration lives in project code.

## Complexity Tracking

No constitution violations require justification.

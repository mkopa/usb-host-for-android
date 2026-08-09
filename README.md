# USB Host for Android

Native-first Android USB Host SDK for professional service, diagnostics, and embedded development.
The current release supports **STLINK-V3 over Android USB OTG** and provides non-destructive access
to [STM32G0B0RET6](https://stm32g0b0ret6.pages.dev) targets without root.

> **Platform status:** Android is supported and hardware-validated. Desktop/native hosts and Rust
> bindings are experimental and remain in validation; they are not compatibility promises yet.

## SDK status

| Area | Status | Notes |
|---|---|---|
| Android USB Host / OTG | **Supported** | Explicit system permission and lifecycle ownership |
| STLINK-V3 debug mode | **Supported** | PIDs `374e`, `374f`, `3753`, `3754`, `3757` |
| STM32G0B0RET6 | **Supported** | Chip ID `0x467`, flash/SRAM identification and bounded reads |
| Java / Kotlin consumers | **Supported** | Small lifecycle-safe API |
| Android C++ consumers | **Supported** | Stable C ABI published through Prefab |
| Windows/Linux/macOS | Experimental | Portable core available; host backends are in validation |
| Rust | Experimental | C ABI is bindgen-ready; first-party crate is not released |

Recorded hardware evidence currently covers a Samsung Galaxy A54, Android USB OTG, STLINK-V3
`0483:3754`, and an STM32G0B0RET6 demonstration board. See
[hardware validation](docs/hardware/README.md).

## Capabilities

- Discovers supported STLINK-V3 probes through Android `UsbManager`.
- Uses the standard Android USB permission prompt; no root or `/dev/bus/usb` scanning.
- Wraps a duplicated `UsbDeviceConnection` file descriptor with libusb.
- Reuses pinned upstream stlink protocol/SWD logic through a narrow Android adapter.
- Identifies STM32G0B/G0C device ID `0x467` and the STM32G0B0RET6 memory layout.
- Performs bounded and validated read-only flash/SRAM reads up to 1 MiB per API call.
- Exposes Java, C17 ABI, C++/Prefab, and future Rust-friendly boundaries.
- Serializes operations, maps errors to stable codes, and closes sessions idempotently.

The public API exposes **no erase, program, option-byte write, target-memory write, register write,
reset, halt, run, step, or trace operation**.

## Architecture

```text
Android app
  └─ UsbManager permission + UsbDeviceConnection ownership
      └─ Java SDK / JNI adapter
          └─ stable C ABI (usbhost/usbhost.h)
              └─ portable C++ session core
                  └─ Android STLINK-V3 adapter
                      ├─ libusb 1.0.30
                      └─ stlink (pinned revision)
```

Platform code is deliberately thin. Session state, validation, ownership rules, range checks, and
the public error model live below JNI so the same contract can be consumed by native C++ and future
Rust integrations.

## Requirements

- Android API 23+ device with USB Host/OTG support
- Android SDK 35
- Android NDK `28.2.13676358`
- CMake `3.22.1`
- Gradle 9.4.1 / Android Gradle Plugin 9.2.1
- JDK 17+
- Recursive Git submodules

## Get started

```powershell
git clone --recurse-submodules <repository-url>
cd usb-host-for-android
./gradlew.bat :usbHostForAndroid:assembleRelease
```

For a local multi-module Android project:

```groovy
dependencies {
    implementation project(':usbHostForAndroid')
}
```

The library publication coordinates are prepared as
`dev.usbhost:usb-host-for-android:0.1.0-SNAPSHOT`; no public Maven repository is claimed.

## Java usage

The application owns the permission prompt. Perform open/connect/read operations on a worker thread:

```java
List<StlinkDevice> probes = StlinkProber.findAll(usbManager);
StlinkDevice probe = probes.get(0); // select explicitly in production

try (StlinkSession session = probe.open(usbManager)) {
    TargetInfo target = session.connectTarget();
    byte[] vectorTable = session.readMemory(target.getFlashBase(), 256);
}
```

The complete Android permission flow is in `usbHostExample`.

## Native C++ usage

The AAR publishes a Prefab package named `usbhost`:

```cmake
find_package(usbhost REQUIRED CONFIG)
target_link_libraries(your_native_target PRIVATE usbhost::usbhost)
```

```cpp
#include <usbhost/usbhost.h>
```

Android grants permission and opens the USB connection before its descriptor is passed to
`usbhost_open_stlink_v3_fd`. Native code duplicates the descriptor; the caller retains ownership of
the original connection. See [native API and Rust preparation](docs/native-api.md).

## Device compatibility

Supported vendor ID is `0x0483`. Supported STLINK-V3 debug-mode product IDs are `0x374e`, `0x374f`,
`0x3753`, `0x3754`, and `0x3757`. Firmware-update PID `0x374d`, ST-Link V1/V2, and V2-1 are not
supported by this release.

When multiple supported probes are attached, the SDK returns all candidates and the application
must select one explicitly.

## Validation

```powershell
./gradlew.bat clean test assembleDebug
cmake -S native-tests -B build/native-tests
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

Automated tests use fake transports and never prove physical-device compatibility. Hardware claims
must have a dated, redacted record under `docs/hardware/`.

## Repository layout

| Path | Purpose |
|---|---|
| `usbHostForAndroid/` | Publishable Android AAR, Prefab headers, native core and adapters |
| `usbHostExample/` | Minimal permission and integration example |
| `native-tests/` | Host-native contracts using fake transports |
| `third_party/` | Immutable libusb and stlink Git submodules |
| `docs/` | Native API and redacted hardware evidence |
| `specs/` | Spec Kit requirements, plans, contracts, and tasks |

## Dependencies and license

The project is MIT licensed. libusb is LGPL-2.1-or-later and remains a replaceable shared library;
stlink is BSD-3-Clause. Exact revisions, provenance, and packaging details are documented in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

# USB Host for Android

Native-first Android USB Host SDK for professional service, diagnostics, and embedded development.
The current release supports **STLINK-V3 over Android USB OTG** and provides non-destructive access
to [STM32G0B0RET6](https://stm32g0b0ret6.pages.dev) targets without root.

> **0.1.0 release candidate:** the public API now uses the exact namespace
> `info.marcin.usbhost`. Maven Central publication is configured but intentionally remains pending
> until the tested `dev` branch is explicitly approved for promotion to `main`.

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
| Generic USB transport | **Available** | Descriptor/configuration/claim plus control, bulk, interrupt primitives |
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
git clone --recurse-submodules https://github.com/mkopa/usb-host-for-android.git
cd usb-host-for-android
./gradlew.bat :usbHostForAndroid:assembleRelease
```

For the current source checkout:

```groovy
dependencies {
    implementation project(':usbHostForAndroid')
}
```

After the approved 0.1.0 release is published to Maven Central, consumers use:

```kotlin
repositories {
    google()
    mavenCentral()
}

dependencies {
    implementation("info.marcin.usbhost:usb-host-for-android:0.1.0")
}
```

Google Maven remains a dependency source for Android platform libraries. Maven Central is the
publication repository; the Gradle Plugin Portal is not used because this project publishes an AAR,
not a Gradle plugin.

## Kotlin usage

The application owns the permission prompt. Perform open/connect/read operations on a worker thread:

```kotlin
val probes = StlinkProber.findAll(usbManager)
val probe = probes.first() // select explicitly in production

probe.open(usbManager).use { session ->
    val target = session.connectTarget()
    val vectorTable = session.readMemory(target.flashBase, 256)
}
```

The complete lifecycle-safe permission flow and polished Compose programmer console are in
`usbHostExample`. The example shows probe/target facts and a formatted 256-byte flash preview; it
contains no write, erase, reset, halt, run, step, or register-mutation command.

## Generic transport

Applications and protocol adapters can use `info.marcin.usbhost.transport` after Android
`UsbManager` grants permission and opens a caller-owned `UsbDeviceConnection`:

```kotlin
withContext(Dispatchers.IO) {
    GenericUsbDevice.open(device, connection).use { usb ->
        val descriptors = usb.configurations
        // Select, claim, and transfer only according to a validated protocol adapter.
    }
}
connection.close()
```

Blocking lifecycle and transfer calls reject the main thread. The library owns only a duplicated
native file descriptor; it does not close the application connection. Control transfers are limited
to 65,535 bytes, bulk/interrupt transfers to 1 MiB, timeouts to 1–60,000 ms, and close to a two-second
cleanup bound. Explicit configuration selection requires zero claims and invalidates earlier
endpoint snapshots.

These primitives do not by themselves claim compatibility with serial, DFU, CMSIS-DAP, HID,
printers, analyzers, other programmers, or arbitrary desktop libusb applications. See the complete
[generic transport guide](docs/transport.md) and [minimal adapter example](docs/transport-adapter-example.md).

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
$env:JAVA_HOME='C:\Program Files\Java\jdk-17'
./gradlew.bat clean test assembleDebug
./gradlew.bat :usbHostForAndroid:verifyReleasePublication
./gradlew.bat -p smoke-tests/android-consumer :consumer:assembleDebug
cmake -S native-tests -B build/native-tests
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

Automated tests use fake transports and never prove physical-device compatibility. Hardware claims
must have a dated, redacted record under `docs/hardware/`.

## Reproducible build container

```powershell
docker build -t usb-host-android-runner docker/android-runner
docker run --rm -v "${PWD}:/workspace" -w /workspace usb-host-android-runner
```

Automation publishes the same pinned image to
`ghcr.io/mkopa/usb-host-android-runner:latest`. An optional Docker Hub mirror is enabled only when
its repository secrets exist.

## Branch and release flow

Create task branches from `dev` and merge them back to `dev` through pull requests. Only an
explicitly approved promotion from `dev` reaches release-only `main`; an exact `v0.1.0` tag on that
main commit starts signed Maven Central publication. See [RELEASING.md](RELEASING.md).

## Repository layout

| Path | Purpose |
|---|---|
| `usbHostForAndroid/` | Publishable Android AAR, Prefab headers, native core and adapters |
| `usbHostExample/` | Minimal permission and integration example |
| `smoke-tests/android-consumer/` | Detached Maven consumer compile contract |
| `native-tests/` | Host-native contracts using fake transports |
| `docker/android-runner/` | Pinned JDK/Android/NDK/CMake build image |
| `third_party/` | Immutable libusb and stlink Git submodules |
| `docs/` | Native API and redacted hardware evidence |
| `specs/` | Spec Kit requirements, plans, contracts, and tasks |

## Dependencies and license

The project is MIT licensed. libusb is LGPL-2.1-or-later and remains a replaceable shared library;
stlink is BSD-3-Clause. Exact revisions, provenance, and packaging details are documented in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

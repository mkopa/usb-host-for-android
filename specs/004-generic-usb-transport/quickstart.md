# Quickstart: Validate Generic USB Transport

This guide validates the completed feature. It does not publish a release or perform device writes,
erase, reset, firmware update, or target mutation.

## Prerequisites

- JDK 17 and the repository's Gradle wrapper
- Android SDK/NDK versions pinned by the project
- CMake 3.22.1 and a host C/C++ toolchain
- Recursive submodules checked out
- For GitHub delivery only: `gh` authenticated with `WRITE` or higher on the repository
- GitHub Actions remain disabled; every merge requires recorded local verification

## 1. Prepare sources

```powershell
git submodule update --init --recursive
git status --short
```

Expected: submodules are present and no generated binary is tracked.

## 2. Run deterministic native contracts

```powershell
cmake -S native-tests -B build/native-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native-tests --config Debug --parallel 2
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

Expected: descriptor, configuration, claim, transfer, cancellation, disconnect, race, ABI, and
STLINK regression contracts pass against the scripted fake backend. No physical USB device is
required.

## 3. Validate managed API and Android packaging

```powershell
./gradlew.bat --no-daemon :usbHostForAndroid:test :usbHostForAndroid:lint
./gradlew.bat --no-daemon :usbHostForAndroid:assembleRelease
./gradlew.bat --no-daemon :usbHostForAndroid:verifyReleasePublication
./gradlew.bat --no-daemon -p smoke-tests/android-consumer :consumer:assembleDebug
```

Expected: tests prove worker-thread guards, immutable values, ownership, boundary validation, and
close behavior. The AAR contains `info.marcin.usbhost.transport`; Prefab exposes
`usbhost/transport.h`; the previous stable STLINK API remains present.

## 4. Verify public compatibility and cleanliness

```powershell
pwsh ./scripts/verify-publication.ps1
git diff --check
```

Expected: previous-baseline Java/C/C++ consumers compile, status values and structure prefixes are
unchanged, exports match the baseline, and no local paths, sensitive identifiers, private branding,
or unsupported device-class claims appear in public artifacts.

## 5. Optional non-destructive Android smoke test

1. Connect a generic demonstration USB device through OTG.
2. Let the application use `UsbManager` to request permission and open `UsbDeviceConnection`.
3. On a worker thread, open `GenericUsbDevice`, inspect descriptors and additional-descriptor records,
   claim/release one interface only when documented safe, and close the generic session.
4. Verify the application-owned `UsbDeviceConnection` remains valid until the application closes it.
5. Disconnect during a safe read/inspection operation and verify cancellation completes within 2 s.
6. Record only sanitized evidence using `docs/hardware/evidence-template.md`.

Do not issue vendor/class requests unless their non-destructive behavior is documented for the test
device. A successful generic smoke test does not establish serial, DFU, HID, CMSIS-DAP, printer,
analyzer, or programmer support.

## 6. Validate GitHub task delivery readiness

```powershell
gh auth status
gh repo view mkopa/usb-host-for-android --json viewerPermission,hasIssuesEnabled
```

Expected before implementation: issues are enabled and `viewerPermission` is `WRITE`, `MAINTAIN`, or
`ADMIN`. After `$speckit-tasks`, every checkbox task is synchronized to one issue before its
`feat/<issue>-<task>-<slug>` branch starts. Every PR targets `dev`, links and closes its issue, and is
merged only after the required local commands pass and their results are recorded in both the issue
and PR. Do not enable or dispatch GitHub Actions unless the maintainer explicitly restores them.

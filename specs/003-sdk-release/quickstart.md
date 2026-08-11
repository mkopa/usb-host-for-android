# Quickstart: Validate the 0.1.0 Release Candidate

## Prerequisites

- JDK 17
- Android SDK with platform 35 and build-tools 35.0.0 or newer
- Android NDK `28.2.13676358`
- CMake 3.22.1
- Git with recursive submodules

## Checkout and verify

```powershell
git clone --recurse-submodules https://github.com/mkopa/usb-host-for-android.git
cd usb-host-for-android
git switch dev
./gradlew.bat --no-daemon clean test lint assembleDebug assembleRelease
```

Run native host contracts:

```powershell
cmake -S native-tests -B build/native-tests
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

## Inspect a local publication without uploading

```powershell
./gradlew.bat :usbHostForAndroid:publishAllPublicationsToReleaseCandidateRepository
./gradlew.bat :usbHostForAndroid:verifyReleasePublication
```

The isolated repository is written below `usbHostForAndroid/build/repository`. This command never
uses Central credentials.

## Build in the pinned container

```powershell
docker build -t usb-host-android-runner docker/android-runner
docker run --rm -v "${PWD}:/workspace" -w /workspace usb-host-android-runner `
  ./gradlew --no-daemon test lint assembleRelease
```

## Run the Kotlin sample

```powershell
./gradlew.bat :usbHostExample:installDebug
```

Attach a compatible probe through Android USB OTG, open **USB Host Programmer**, grant the system
permission, connect, and request the read-only preview. The sample exposes no target mutation
operation.

## Consumer smoke project

After 0.1.0 is actually published, an external Android project uses:

```kotlin
repositories { mavenCentral() }
dependencies { implementation("info.marcin.usbhost:usb-host-for-android:0.1.0") }
```

Until explicit promotion approval, validate only the local repository or the task/dev build. Do not
merge `dev` to `main`, create `v0.1.0`, or run the live Central publication task.

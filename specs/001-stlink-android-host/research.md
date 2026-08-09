# Research: ST-Link Android Host

## Decision 1: Android owns USB discovery and permission

**Decision**: Enumerate and select devices through `UsbManager`, open a `UsbDeviceConnection`, and
pass its native file descriptor through JNI. Native code duplicates the descriptor, initializes
libusb with device discovery disabled, and wraps the duplicate with `libusb_wrap_sys_device`.

**Rationale**: Android explicitly exposes the descriptor for native access. libusb documents this
unrooted Android flow and requires the descriptor to remain open until the libusb handle closes.
This avoids root, protected path traversal, fragile device matching, and double enumeration.

**Alternatives considered**:

- Direct libusb enumeration: rejected because unrooted applications lack general USB filesystem
  authority and Android must grant permission per device.
- Reimplement all USB transfers with `UsbDeviceConnection.bulkTransfer`: rejected because it would
  fork stlink's proven transport behavior and reduce native portability.

**Sources**:

- https://developer.android.com/reference/android/hardware/usb/UsbDeviceConnection#getFileDescriptor()
- https://github.com/libusb/libusb/blob/master/android/README
- https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html

## Decision 2: Adapt, do not fork, stlink's USB backend

**Decision**: Pin upstream stlink and compile its library sources, replacing only the device-open and
chip-database seams with small Android adapters. The transport commands and target logic remain
upstream code. The adapter accepts an already wrapped libusb handle and performs programmer-only
initialization; target connection is an explicit later operation.

**Rationale**: Upstream `stlink_open_usb` combines host enumeration, programmer open, and target
connection. Android already performed enumeration and permission, and programmer open must succeed
even when the target is absent. A narrow adapter preserves upstream protocol code without maintaining
a broad source fork.

**Alternatives considered**:

- Call `stlink_open_usb` unchanged: rejected because it enumerates devices and cannot use the
  Android-authorized descriptor.
- Reimplement ST-Link commands: rejected due to protocol risk and unnecessary maintenance.
- Carry a full permanent fork: rejected because upstream updates would be difficult to audit.

**Source**: https://github.com/stlink-org/stlink

## Decision 3: Compile one target descriptor into v1

**Decision**: Replace stlink's filesystem-scanned chip database with a constant descriptor for
STM32G0B/G0C device ID `0x467`. The descriptor defines G0 flash behavior, flash-size register
`0x1fff75e0`, 2 KiB pages, 144 KiB SRAM, and dual-bank capability.

**Rationale**: Android library assets are not ordinary filesystem paths and v1 supports exactly one
MCU family. A constant eliminates extraction and path lifecycle while keeping target interpretation
identical to the pinned upstream chip description.

**Alternatives considered**:

- Extract every `.chip` file to application storage: rejected as unnecessary I/O and lifecycle
  complexity for one supported target.
- Embed the full chip database in generated C: deferred until more targets are specified.

**Sources**:

- https://github.com/stlink-org/stlink/blob/develop/config/chips/G0Bx_G0Cx.chip
- https://www.st.com/en/microcontrollers-microprocessors/stm32g0b0re.html

## Decision 4: Stable C ABI beneath Java/JNI

**Decision**: Publish opaque 64-bit session handles, fixed numeric status values, size-prefixed
fixed-width structures, caller-owned read buffers, and thread-local diagnostic text. JNI calls this
same ABI rather than reaching directly into C++ classes.

**Rationale**: C++ applications can link through Prefab immediately, while Rust can bind the same
contract later. No C++ exceptions, STL types, JNI objects, or Android types cross the ABI.

**Alternatives considered**:

- JNI-only native API: rejected because it blocks direct native clients and couples all behavior to
  Android classes.
- Public C++ ABI: rejected because compiler/STL ABI stability is weaker and Rust FFI is harder.

## Decision 5: Read-only first release

**Decision**: Expose discover, open, connect, target information, memory read, and close. Do not
expose reset, halt, run, register writes, erase, flash programming, or option-byte changes.

**Rationale**: It proves the complete data path while making accidental destructive operations
impossible through the public contract. Write behavior needs separate authorization, recovery, and
hardware test requirements.

## Decision 6: Dependency and build versions

**Decision**: Use AGP 9.2.1 with Gradle 9.4.1, compile SDK 35, min SDK 23, NDK r28.2.13676358,
CMake 3.22.1, libusb v1.0.30, and an immutable stlink develop revision. Build libusb as a separate
shared library (LGPL replacement-friendly) and statically include BSD-licensed stlink into the
project shared library. Ship license notices.

**Rationale**: Versions match the installed Windows 11 toolchain and the adjacent reference project.
AGP 9.2 requires Gradle 9.4.1. CMake through `externalNativeBuild` uses the supported NDK toolchain.
Pinned revisions avoid dynamic dependency drift.

**Sources**:

- https://developer.android.com/build/releases/about-agp
- https://developer.android.com/ndk/guides/cmake
- https://github.com/libusb/libusb/tree/v1.0.30
- https://github.com/stlink-org/stlink/commit/6a1d36530b7ec0004498fb80c947c46e6c537134

## Decision 7: Concurrency and descriptor ownership

**Decision**: Each session serializes operations with one mutex. Native open duplicates the Android
descriptor; native close first closes stlink/libusb, then closes that duplicate. Java retains and
finally closes its `UsbDeviceConnection`. Close is idempotent and waits for an in-flight operation.

**Rationale**: This makes ownership explicit, prevents use-after-close, and satisfies libusb's rule
that the wrapped descriptor remains open for the handle lifetime.

**Alternatives considered**:

- Borrow the original descriptor only: rejected because external closure could invalidate a session.
- Permit concurrent transfers: rejected because stlink state and buffers are session-local and not
  designed for overlapping commands.

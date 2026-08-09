# Feature Specification: ST-Link Android Host

**Feature Branch**: `001-stlink-android-host`

**Created**: 2026-08-08

**Status**: Approved

**Input**: Android USB host library inspired by `usb-serial-for-android`, with a portable native
boundary. The first supported programmer is ST-Link V3 and the first target is STM32G0B0RET6.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Discover and Open a Programmer (Priority: P1)

An Android application discovers attached ST-Link V3 programmers, distinguishes supported devices,
and opens a selected programmer after Android USB permission has been granted.

**Why this priority**: No target communication is possible until applications can reliably select,
authorize, open, and close the programmer.

**Independent Test**: Attach a supported programmer and verify that it is listed, opens after
permission is granted, reports its identity, and releases the connection when closed.

**Acceptance Scenarios**:

1. **Given** a supported ST-Link V3 is attached, **When** the application probes attached USB
   devices, **Then** the programmer is returned with its USB identity and support status.
2. **Given** Android USB permission and an unopened supported programmer, **When** the application
   opens it, **Then** an active session is returned and the programmer version is available.
3. **Given** permission is absent, **When** the application attempts to open the programmer,
   **Then** opening fails with a permission-specific error and no native session remains allocated.
4. **Given** an active session, **When** it is closed more than once, **Then** closure is safe and
   the underlying USB connection is released exactly once.

---

### User Story 2 - Identify the STM32 Target (Priority: P1)

An application connects through SWD to an STM32G0B0RET6 and obtains enough target information to
confirm that the expected device is present before any memory operation.

**Why this priority**: A positive target identity is the minimum useful end-to-end proof that the
Android device, programmer, and target MCU communicate correctly.

**Independent Test**: Connect a powered STM32G0B0RET6 to ST-Link V3, open a session, and verify the
reported chip family, chip identifier, flash size, SRAM size, and target voltage.

**Acceptance Scenarios**:

1. **Given** an open programmer and a correctly wired, powered STM32G0B0RET6, **When** target
   connection is requested, **Then** the target is identified as the supported STM32G0B0/G0C0
   family and its memory characteristics are returned.
2. **Given** no target power or invalid SWD wiring, **When** connection is requested, **Then** the
   operation fails within the configured timeout with an actionable target-connection error.
3. **Given** a connected but unsupported target, **When** identification completes, **Then** the
   application receives an unsupported-target result without performing a write or erase.

---

### User Story 3 - Read Target Memory Safely (Priority: P2)

An application reads a bounded byte range from a connected STM32G0B0RET6 without changing target
flash, option bytes, or RAM.

**Why this priority**: A deterministic read proves practical communication while keeping the first
release non-destructive.

**Independent Test**: Read known bytes from device flash and compare them with bytes previously
verified using an independent programmer tool.

**Acceptance Scenarios**:

1. **Given** a connected supported target and a valid readable range, **When** memory is requested,
   **Then** the exact requested bytes are returned in address order.
2. **Given** an invalid, overflowing, or unreasonably large range, **When** memory is requested,
   **Then** it is rejected before any USB transfer starts.
3. **Given** a disconnect during a read, **When** the transfer fails, **Then** the session becomes
   unusable, an actionable disconnection error is returned, and subsequent close remains safe.

---

### User Story 4 - Integrate Through a Stable Native Boundary (Priority: P3)

A native client integrates programmer discovery results, session lifecycle, target information,
memory reads, and structured errors through a language-neutral contract.

**Why this priority**: The initial Android API must serve C++ now without tying the portable core to
the Android runtime, and it must leave a direct path for a later Rust implementation.

**Independent Test**: A native contract test creates and closes a fake session, requests target
information and memory, and verifies stable result and error values without Android framework types.

**Acceptance Scenarios**:

1. **Given** a conforming native client, **When** it uses the published contract, **Then** no Android
   framework object or language-specific exception is required outside the Android adapter.
2. **Given** an older client and a compatible library update, **When** the client uses existing
   operations, **Then** numeric status values, structure layout rules, and ownership remain valid.

### Edge Cases

- Multiple supported programmers are attached and the caller must select one deterministically.
- The device is detached between discovery, permission grant, and open.
- Android grants permission but opening the USB device returns no connection.
- The supplied file descriptor is invalid or is closed externally during a session.
- ST-Link is in firmware-update mode instead of debug mode.
- The target voltage is missing or outside a usable range.
- The target is held in reset, protected, sleeping, or not responding at the requested SWD speed.
- A memory request crosses an address boundary or exceeds the configured per-request limit.
- Two operations are attempted concurrently on the same session.
- Close races with an in-flight operation.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The library MUST identify supported ST-Link V3 debug-mode USB devices from the set of
  devices already enumerated by Android.
- **FR-002**: The library MUST NOT enumerate protected system USB paths or require root access.
- **FR-003**: The library MUST require an Android-granted device connection before creating a native
  programmer session.
- **FR-004**: The library MUST expose stable programmer identities sufficient to distinguish
  multiple attached devices.
- **FR-005**: The library MUST own and document the lifecycle of each active session and MUST support
  idempotent closure.
- **FR-006**: The library MUST serialize operations within one session and MUST reject use after
  closure or terminal disconnection.
- **FR-007**: The library MUST connect to a target using SWD and report connection failures without
  hanging indefinitely.
- **FR-008**: The library MUST recognize STM32G0B0RET6 through the STM32G0B/G0C device identity and
  report chip identifier, flash size, SRAM size, and measured target voltage.
- **FR-009**: The library MUST return an explicit unsupported-target result for other targets in the
  first release.
- **FR-010**: The library MUST read an arbitrary valid byte range from supported target flash or RAM,
  internally splitting transfers when necessary.
- **FR-011**: The library MUST validate memory address, length, overflow, supported regions, and the
  configured maximum request size before starting a read.
- **FR-012**: The initial release MUST NOT expose target erase, flash programming, option-byte writes,
  register writes, execution control, or reset operations through its public API.
- **FR-013**: The library MUST map permission, argument, USB transport, timeout, disconnection,
  programmer, target, unsupported-device, and internal failures to stable structured errors.
- **FR-014**: Diagnostic output MUST omit serial numbers and memory contents by default.
- **FR-015**: Blocking programmer operations MUST be rejected on the Android main thread or exposed
  only through an API that runs them off that thread.
- **FR-016**: The native contract MUST define ownership, structure sizing, status values, and error
  message lifetime without Android framework types.
- **FR-017**: The public Android API MUST be callable from both Java and Kotlin applications.
- **FR-018**: Pure session, validation, state, and error behavior MUST be testable without physical
  USB or STM32 hardware.
- **FR-019**: Hardware support claims MUST record the Android version/device, ST-Link variant and
  firmware, target identity, wiring conditions, and observed result.
- **FR-020**: The first release MUST support one active operation per session and MUST document that
  simultaneous sessions are not guaranteed.

### Key Entities

- **Programmer Descriptor**: Stable identity and support classification for an attached ST-Link.
- **Programmer Session**: Exclusive, closeable association between an Android USB connection and one
  ST-Link device, including lifecycle and terminal failure state.
- **Target Descriptor**: Identified MCU family, chip identifier, memory sizes, and target voltage.
- **Memory Range**: Start address and byte length validated against the target descriptor and request
  limits.
- **Operation Result**: Success value or a stable error category with optional diagnostic context.
- **Hardware Evidence Record**: Environment and result proving a hardware-dependent support claim.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On supported hardware, an application can discover, authorize, open, identify, and
  close one ST-Link V3 session in under 10 seconds, excluding user response time to permission UI.
- **SC-002**: With a correctly connected STM32G0B0RET6, 20 consecutive target-identification runs
  report the same chip identifier and memory sizes with no application restart.
- **SC-003**: Reads of 1 byte, 4 bytes, 1 KiB, and 64 KiB return byte-for-byte correct data in 20
  consecutive runs for each size on supported hardware.
- **SC-004**: Permission denial, unsupported device, missing target, invalid range, detach during
  transfer, timeout, and repeated close each produce the documented result in automated or recorded
  hardware tests.
- **SC-005**: All behavior not dependent on real USB timing passes automated tests on a development
  machine with no programmer connected.
- **SC-006**: A sample Java client and a native contract test both complete the supported workflow
  without relying on undocumented ownership or error behavior.
- **SC-007**: No public operation in the initial release can erase or modify target memory.

## Assumptions

- The Android device supports USB host mode and can power or communicate with an externally powered
  ST-Link V3.
- The application, not the library, owns the user-facing USB permission request flow.
- ST-Link V3 is in a normal debug configuration; firmware-update-only mode is reported as unsupported.
- STM32G0B0RET6 is connected through SWD with common ground and valid target reference voltage.
- The first release is a library plus validation sample, not a complete programmer UI.
- Target writes, erase, flashing, debugging, tracing, ST-Link V1/V2, and a concrete Rust binding are
  out of scope for this feature.
- Hardware-independent tests are required; physical hardware validation is recorded separately when
  the required Android device, ST-Link V3, and STM32G0B0RET6 are available.

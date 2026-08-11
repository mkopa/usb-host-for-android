# Feature Specification: Generic USB Transport

**Feature Branch**: `feature/generic-usb-transport`

**Created**: 2026-08-11

**Status**: Draft

**Input**: Expose a public `info.marcin.usbhost.transport` module that turns an Android-authorized
USB file descriptor into a generic device/interface model and libusb-backed control, bulk, and
interrupt transfers. The module is the reusable base for protocol adapters such as serial, DFU,
CMSIS-DAP, HID, printers, analyzers, and programmers; STLINK-V3 remains the first safe product API.

## Clarifications

### Session 2026-08-11

- Q: How should `close()` behave while a blocking USB transfer is active on the same session? → A: Cancel the active transfer, wait for its safe completion, and release resources within a bounded time.
- Q: How should the v1 public API expose non-standard descriptors needed by future protocol adapters? → A: Expose size-bounded immutable `type + raw bytes` records at their configuration, alternate-setting, or endpoint scope.
- Q: Should v1 let applications explicitly change the active USB configuration before claiming interfaces? → A: Provide explicit configuration selection only while no interfaces are claimed, then refresh active alternate settings and endpoints.
- Q: What hard per-operation limits should the first transport version enforce? → A: Control transfers up to 65,535 bytes, bulk/interrupt transfers up to 1 MiB, timeouts from 1 to 60,000 ms, and `close()` completion within 2 seconds.
- Q: What compatibility level should apply from the first public `info.marcin.usbhost.transport` release? → A: Treat both the managed API and C ABI as stable; incompatible changes require a new major version.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Open an Authorized Generic USB Device (Priority: P1)

An Android developer obtains permission through `UsbManager`, opens a `UsbDeviceConnection`, and
creates a `GenericUsbDevice` without native device discovery. The developer can inspect stable
device, configuration, interface, alternate-setting, and endpoint descriptors and close the session
without leaking the Android connection or its file descriptor.

**Why this priority**: A permission-safe generic session and descriptor model are prerequisites for
every class-specific adapter.

**Independent Test**: With a fake native backend and with one authorized demonstration device,
open from the Android connection, enumerate descriptors, claim and release one interface, close the
generic session twice, and verify the caller still owns and can close its Android connection.

**Acceptance Scenarios**:

1. **Given** an authorized open `UsbDeviceConnection`, **When** the developer opens a generic
   session, **Then** the library duplicates/adopts only its own descriptor and exposes immutable
   descriptor objects under `info.marcin.usbhost.transport`.
2. **Given** no Android permission, a closed connection, or an invalid descriptor, **When** open is
   attempted, **Then** it fails with a stable transport error and creates no live native handle.
3. **Given** multiple interfaces and alternate settings, **When** descriptors are inspected, **Then**
   interface numbers, alternate settings, endpoint addresses, directions, types, and packet sizes
   match the USB descriptors.
4. **Given** a live generic session, **When** an interface is claimed/released or the session is
   closed, **Then** ownership transitions are deterministic and repeated cleanup is safe.
5. **Given** a device with multiple configurations and no claimed interfaces, **When** the developer
   explicitly selects a configuration, **Then** the active interface, alternate-setting, and endpoint
   model is refreshed; selection is rejected while any interface remains claimed.

---

### User Story 2 - Exchange Control, Bulk, and Interrupt Data (Priority: P1)

An adapter author performs bounded synchronous control, bulk, and interrupt transfers through the
generic session, using explicit timeouts and receiving the actual transferred byte count plus stable,
actionable errors.

**Why this priority**: These transfer primitives are the minimum useful transport for the intended
device classes and must be correct before protocol adapters are added.

**Independent Test**: Run deterministic native contract tests against a scripted fake backend for
IN/OUT, short packet, zero-length, timeout, stall, cancellation, disconnect, and invalid endpoint
cases; then run a non-destructive Android smoke test against a permitted demonstration device.

**Acceptance Scenarios**:

1. **Given** a valid setup packet and timeout, **When** a control IN or OUT transfer completes,
   **Then** the caller receives exactly the bytes reported by libusb and no uninitialized data.
2. **Given** a claimed bulk or interrupt endpoint, **When** a transfer completes with a short packet,
   **Then** the result preserves the actual byte count without treating the short packet as failure.
3. **Given** a timeout, stall, disconnect, cancellation, or malformed request, **When** the transfer
   ends, **Then** a stable status identifies the condition and retains bounded native diagnostic
   context without logging device-identifying information.
4. **Given** concurrent calls on one session, **When** transfers and close overlap, **Then** the
   documented serialization and cancellation rules prevent use-after-close and deadlock.

---

### User Story 3 - Build Protocol Adapters on One Stable Boundary (Priority: P2)

A Java/Kotlin, C++, or future Rust adapter author uses the same public transport concepts rather than
reimplementing Android permission or libusb wrapping. Existing STLINK-V3 behavior is routed through
the generic transport without expanding its safe product operations.

**Why this priority**: A stable adapter boundary turns working internals into an extensible library
and proves the design with the existing supported device.

**Independent Test**: Compile a Java/Kotlin adapter fixture and C/C++ ABI consumers, verify exported
symbols and headers from the AAR/Prefab package, and run the existing read-only STLINK-V3 contracts
through the generic open/claim/transfer lifecycle with no regression.

**Acceptance Scenarios**:

1. **Given** the published AAR, **When** a managed adapter imports
   `info.marcin.usbhost.transport`, **Then** it can open, inspect, claim, transfer, release, and close
   without accessing package-private STLINK classes or raw JNI handles.
2. **Given** the Prefab package, **When** C and C++ consumers compile, **Then** opaque handles,
   fixed-width values, caller-owned buffers, and size-versioned structs form a stable C ABI.
3. **Given** the existing STLINK-V3 API, **When** its transport opens and exchanges USB commands,
   **Then** it reuses the generic transport and preserves current read-only safety restrictions.
4. **Given** a future Rust binding, **When** the C header is processed, **Then** it requires no Android
   framework types, C++ exceptions, templates, or ownership of library-allocated transfer buffers.
5. **Given** an adapter compiled against an earlier transport release in the same major version,
   **When** the library is upgraded, **Then** its documented managed API and C ABI remain source and
   binary compatible.

### Edge Cases

- Permission is revoked or the USB device detaches between opening the Android connection and
  wrapping its descriptor.
- A descriptor is malformed, reports zero endpoints, repeats an endpoint address, contains an
  unknown class-specific descriptor, or exceeds the public bound for additional descriptor bytes.
- An interface is already claimed, release is attempted by the wrong session, or alternate setting
  changes invalidate cached endpoint selection.
- Configuration selection is requested while an interface is claimed, or the selected configuration
  disappears after reconnect; the operation fails without changing the active descriptor model.
- A transfer uses endpoint zero incorrectly, mismatches direction/type, requests more than the
  configured maximum, uses a negative/overflowed length, or supplies a timeout outside 1–60,000 ms.
- Close occurs from a second thread while a transfer is blocked, or an Android lifecycle owner is
  destroyed during I/O.
- Multiple generic devices are active and one disconnects; the other sessions remain usable.
- A device supports isochronous endpoints; descriptors remain inspectable but v1 transfer calls
  reject the unsupported transfer type.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The library MUST expose the public managed namespace
  `info.marcin.usbhost.transport` in the existing Android AAR.
- **FR-002**: Android `UsbManager` and the application MUST remain authoritative for discovery,
  permission, and opening `UsbDeviceConnection`; native code MUST NOT scan `/dev/bus/usb`.
- **FR-003**: Opening a generic session MUST duplicate or otherwise isolate the authorized file
  descriptor so ownership of the caller's `UsbDeviceConnection` remains explicit and unchanged.
- **FR-004**: The API MUST expose immutable generic device, configuration, interface,
  alternate-setting, and endpoint descriptors with raw class/subclass/protocol values preserved;
  size-bounded additional descriptors MUST be copied into immutable `type + raw bytes` records at
  their configuration, alternate-setting, or endpoint scope without exposing libusb structures.
- **FR-005**: The API MUST support explicit configuration selection only while no interfaces are
  claimed, refresh active alternate settings and endpoints after a successful selection, support
  explicit interface claim/release and alternate-setting selection, and validate endpoint membership
  against the current active setting; it MUST NOT change configurations implicitly.
- **FR-006**: The API MUST provide control, bulk, and interrupt IN/OUT transfers with explicit
  timeout, caller-supplied buffers, offsets/lengths, and actual transferred byte counts.
- **FR-007**: Version 1 MUST reject isochronous transfer execution with a stable unsupported status;
  descriptor inspection MUST still report isochronous endpoints.
- **FR-008**: A control transfer MUST be limited to 65,535 bytes, a bulk or interrupt transfer to
  1 MiB, and every transfer timeout to 1–60,000 ms; the implementation MUST validate these limits,
  JNI array pin/copy duration, timeout conversion, and integer arithmetic before calling libusb.
- **FR-009**: The API MUST define stable errors for invalid arguments/state, permission, unsupported
  operations/devices, busy, timeout, pipe/stall, cancellation, disconnect, USB failure, and internal
  failure while retaining sanitized diagnostic context.
- **FR-010**: Blocking transfers MUST be documented and guarded against execution on the Android
  main thread.
- **FR-011**: The public transfer API MUST remain synchronous while the native implementation uses
  cancellable libusb operations; `close()` MUST reject new work, cancel any active transfer, wait for
  safe completion within 2 seconds, release resources, and remain idempotent.
- **FR-012**: No serial number, local USB path, raw descriptor dump, or device identifier MUST be
  logged by default.
- **FR-013**: A public native C ABI MUST expose opaque handles, fixed-width fields, caller-owned
  buffers, size-versioned structures, explicit status returns, and no Android framework types.
- **FR-014**: C++ consumers MUST be able to use the C ABI through the published Prefab headers, and
  the ABI MUST remain suitable for generated future Rust FFI bindings.
- **FR-015**: The existing safe STLINK-V3 product API MUST reuse the generic transport lifecycle and
  MUST NOT gain target write, erase, reset, halt, run, step, or other mutating operations as part of
  this feature.
- **FR-016**: Automated tests MUST cover descriptor parsing, ownership, transfers, errors,
  concurrency, cancellation, detach, ABI layout, symbol visibility, and STLINK-V3 regression without
  requiring destructive hardware operations.
- **FR-017**: Public documentation MUST distinguish generic USB transport capability from support for
  a particular USB class or arbitrary desktop libusb application.
- **FR-018**: The feature MUST NOT claim compatibility for serial, DFU, CMSIS-DAP, HID, printers,
  analyzers, or other programmers until a corresponding adapter has its own verified support record.
- **FR-019**: The managed transport API and public C ABI MUST be stable from their first public
  release: changes within one major version MUST be backward-compatible and additive, existing
  status numbers and function semantics MUST NOT change, size-versioned structures MAY gain trailing
  fields, and any incompatible change MUST require a new major version with migration guidance.

### Key Entities

- **Generic USB Device**: A lifecycle-bound session created from an Android-authorized connection,
  with immutable descriptors, an explicitly selected active configuration, claimed-interface state,
  a native opaque handle, and close state.
- **Generic USB Interface**: An interface number plus active alternate setting and its endpoints;
  claim/release belongs to one generic device session.
- **Generic USB Endpoint**: Address, direction, transfer type, maximum packet size, interval, and the
  interface/alternate setting that owns it.
- **Control Request**: Request type, request, value, index, direction, buffer slice, and timeout.
- **Additional Descriptor**: An immutable, size-bounded descriptor type and raw byte payload attached
  to the configuration, alternate setting, or endpoint where libusb reported it.
- **Transfer Result**: Stable status, actual byte count, and sanitized diagnostic context.
- **Protocol Adapter**: Class- or product-specific logic layered over the generic session without
  owning Android discovery or permission.

## Success Criteria *(mandatory)*

- **SC-001**: A consumer opens and enumerates an authorized generic device using no API outside
  `info.marcin.usbhost.transport` and Android's public USB Host API.
- **SC-002**: Deterministic tests cover every control/bulk/interrupt direction and all specified
  terminal errors, exact transfer/timeout boundaries, and the 2-second close bound with zero
  sanitizer, leak, or race failures in supported native test builds.
- **SC-003**: C and C++ contract consumers compile from published Prefab headers and exported-symbol
  inspection finds only the documented transport and existing stable API surface.
- **SC-004**: Existing read-only STLINK-V3 tests and supported hardware smoke checks pass through the
  shared generic transport with no newly exposed mutating operation.
- **SC-005**: Static checks find no native USB device discovery, default sensitive identifier logs,
  Android main-thread blocking path, or undocumented ownership transfer.
- **SC-006**: A minimal adapter fixture reads one additional descriptor, performs descriptor
  selection and one scripted transfer with at most one transport-specific dependency, and contains
  no JNI implementation of its own.
- **SC-007**: Compatibility tests compile the previous-major-baseline managed and C/C++ consumers
  against the candidate artifact and verify unchanged status values, structure prefixes, documented
  symbols, and behavior for every release within the same major version.

## Assumptions

- Version 1 exposes synchronous transfer primitives and applications choose their executor/coroutine
  dispatcher; internally, transfers remain cancellable so `close()` can complete safely within its
  documented bound.
- The module is a public package within the existing AAR and Prefab library, not a second Maven
  coordinate in this increment.
- The first published transport API establishes its compatibility baseline even if the enclosing
  library has not yet reached version 1.0.
- The current pinned libusb implementation remains internal; consumers receive the stable transport
  API rather than unrestricted access to libusb symbols or structures.
- Android API 23+ and the currently published ABIs remain supported.
- Isochronous execution, kernel-driver detach, USB hotplug discovery, desktop binary compatibility,
  and class-specific protocol implementations are outside this feature.

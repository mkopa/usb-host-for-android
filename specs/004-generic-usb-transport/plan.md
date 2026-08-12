# Implementation Plan: Generic USB Transport

**Branch**: `004-generic-usb-transport` | **Date**: 2026-08-11 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/004-generic-usb-transport/spec.md`

## Summary

Implement the public `info.marcin.usbhost.transport` API inside the existing `usbHostForAndroid`
AAR. Android `UsbManager` remains responsible for discovery, permission, and connection lifecycle;
the library duplicates the authorized file descriptor and wraps it with the existing pinned libusb
without native discovery. A portable C++17 session core exposes immutable descriptor snapshots,
explicit configuration/interface state, and synchronous control/bulk/interrupt calls over an
internally asynchronous, cancellable backend. JNI remains a thin Android/managed adapter, and the
additive C ABI is published through the existing Prefab module for C++ and future Rust consumers.
The existing safe STLINK-V3 API is migrated to the shared transport without breaking or expanding
its mutating capabilities.

## Technical Context

**Language/Version**: Java 17 public API, C17 ABI, C++17 core, CMake 3.22.1

**Primary Dependencies**: Android USB Host API, JNI, existing pinned libusb 1.0.30 sources, existing
STLINK submodule; no new runtime dependency

**Storage**: No persistent product storage; immutable in-memory descriptor snapshots, bounded
caller-owned transfer buffers, and test fixtures only

**Testing**: Local JUnit 4 public API contracts, local CTest host-native fake-backend contracts,
ABI/layout and export inspection, Android lint/assembly, detached Maven/Prefab consumer builds, and
optional recorded non-destructive hardware validation; GitHub Actions remain disabled temporarily

**Target Platform**: Android API 23+; arm64-v8a, armeabi-v7a, and x86_64 AAR consumers; host-native
C/C++ contract builds run locally before each merge

**Project Type**: Existing Android AAR with Java/Kotlin API, JNI, shared native library, and Prefab
C ABI

**Performance Goals**: Control payload at most 65,535 bytes; bulk/interrupt payload at most 1 MiB;
timeout 1–60,000 ms; cancelled `close()` completes within 2 seconds; no blocking call on Android's
main thread; different device sessions may progress concurrently

**Constraints**: Stable managed API and C ABI from first publication; append-only status values and
size-versioned structs; caller owns `UsbDeviceConnection`; native session owns only its duplicated
FD; libusb stays internal; no `/dev/bus/usb` scanning; no isochronous execution; no implicit USB
configuration change; no sensitive device logging

**Scale/Scope**: Multiple simultaneous devices; every configuration/interface/alternate setting;
standard and bounded additional descriptors; one serialized active transfer per session; control,
bulk, and interrupt IN/OUT; STLINK-V3 as the only adapter with an existing support claim

## Constitution Check

*GATE: Passed before Phase 0 research and passed again after Phase 1 design.*

- **Portable core, thin adapters — PASS**: `transport/` core types, state machines, validation, and
  fake backend contain no Android/JNI types. Android connection and Looper handling stay in Java/JNI;
  libusb FD wrapping stays in the Android backend.
- **Explicit stable boundaries — PASS**: Contracts define FD ownership, immutable copies, buffer
  ownership, threading, cancellation, 2-second close, errors, ABI layout, and compatibility.
- **Hardware-safe evidence — PASS**: Automated acceptance uses a scripted fake backend. Hardware
  validation is optional, recorded, sanitized, non-destructive, and cannot establish support for an
  unimplemented class adapter.
- **Minimal reviewable delivery — PASS**: Work remains inside the existing AAR, shared library, and
  Prefab module. No new runtime dependency or second Maven coordinate is introduced.
- **Licensing and provenance — PASS**: Existing pinned libusb/STLINK sources and notices are reused;
  no copied third-party implementation or new license enters the feature.
- **Public repository policy — PASS**: Generic examples, sanitized evidence, mandated commit identity,
  and staged-content inspection are explicit delivery gates.
- **Delivery workflow — PASS WITH EXECUTION PREREQUISITE**: Each generated task maps to one GitHub
  issue, `feat/<issue>-<task>-<slug>` branch, PR into `dev`, issue status updates, and merge after all
  local verification gates pass. GitHub Actions remain disabled until explicitly restored, so every
  PR records exact local commands/results. The authenticated actor has the required write access.

## Architecture and Boundaries

```text
Android application
  UsbManager → permission → UsbDeviceConnection (application-owned)
                                │ getFileDescriptor()
                                ▼
info.marcin.usbhost.transport   Java 17, immutable values, worker-thread guard
                                │ JNI primitives and caller byte[] slices
                                ▼
usbhost transport C ABI        opaque handles, fixed widths, struct_size
                                │
                                ▼
portable C++17 core             session/descriptor/claim/transfer state machines
              │                                  │
              │ tests                            │ Android runtime
              ▼                                  ▼
scripted fake backend            libusb Android backend + shared event loop
                                                    │ duplicated authorized FD
                                                    ▼
                                            USB device endpoints

Existing STLINK-V3 adapter → same internal transport backend → unchanged public STLINK API
```

### Runtime decision

The managed API is synchronous so adapter authors control their executor or coroutine dispatcher.
Internally, Android transfers use libusb asynchronous submissions and a process-wide reference-counted
runtime with one event thread. Each session serializes its own operations, records the active transfer,
and can request `libusb_cancel_transfer()` during close. Completion callbacks publish actual length
and terminal status before resources are released. The fake backend implements the same completion
contract deterministically without threads or physical USB.

### Ownership decision

`GenericUsbDevice.open` reads but never owns the application's `UsbDeviceConnection`. Native open
duplicates its FD; libusb wraps the duplicate and does not own it. Close cancels/waits for the active
transfer, releases claimed interfaces, closes the libusb handle/context reference, then closes the
duplicate exactly once. Existing `StlinkSession` retains its current managed connection-ownership
semantics to avoid a breaking API change while reusing the new internal transport.

### Descriptor and configuration decision

Opening snapshots the device descriptor and every available configuration. Standard fields are
owned values. Additional configuration, alternate-setting, and endpoint descriptors are copied into
bounded immutable `type + raw bytes` records. One configuration is active. Explicit configuration
selection is rejected while any interface is claimed and refreshes active alternate settings and
endpoints on success. Endpoint objects carry their session snapshot generation so stale endpoints
fail validation after configuration or alternate-setting changes.

### Compatibility decision

The public managed package and C ABI are stable from first release even before library 1.0. Existing
status numbers and semantics never change, new statuses append, public structs start with
`struct_size`, new fields append, functions use C linkage, and caller buffers avoid allocator
coupling. JNI symbols are implementation details; documented Java/Kotlin and C functions are the
compatibility boundary.

## Project Structure

### Documentation (this feature)

```text
specs/004-generic-usb-transport/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── transport-api.md
│   ├── managed-api.md
│   ├── native-c-abi.md
│   └── github-delivery.md
└── tasks.md                 # regenerated later by $speckit-tasks
```

### Source Code (repository root)

```text
usbHostForAndroid/
├── build.gradle
└── src/
    ├── main/java/info/marcin/usbhost/
    │   ├── transport/                    # public managed API and private JNI bridge
    │   └── *.java                        # compatible existing STLINK API
    ├── main/cpp/
    │   ├── include/usbhost/
    │   │   ├── usbhost.h                 # existing umbrella ABI
    │   │   └── transport.h               # additive transport ABI
    │   ├── transport/                    # portable core interfaces and state machines
    │   ├── android/                      # libusb authorized-FD backend/runtime
    │   ├── jni/transport_jni.cpp         # thin managed adapter
    │   ├── stlink/                       # existing adapter migrated internally
    │   └── tests/                        # host fake/contract tests
    └── test/java/info/marcin/usbhost/transport/

native-tests/                              # host CMake entrypoint and ABI consumers
smoke-tests/android-consumer/              # detached published-AAR consumer
scripts/                                   # publication and task/issue verification
.github/
├── ISSUE_TEMPLATE/
├── pull_request_template.md
└── workflows/                            # retained but disabled at repository level
```

**Structure Decision**: Extend the existing `usbHostForAndroid` module, `libusbhost.so`, AAR, Maven
coordinate, and Prefab package. Keep protocol-neutral code in `transport/`; keep Android/libusb FD
authority in `android/`; preserve existing public STLINK files and signatures.

## Implementation Phases

### Phase 1 — Contract and deterministic foundation

1. Freeze managed and C ABI signatures, status additions, limits, descriptor bounds, and symbol
   baseline before implementation.
2. Add portable value types, generation-safe session registry, backend interface, scripted fake, and
   error sanitization/mapping.
3. Add failing ownership, ABI, descriptor, state-machine, and boundary tests.

### Phase 2 — Authorized session and descriptor model

1. Add the Android libusb runtime with no discovery, duplicated FD wrapping, and one event loop.
2. Snapshot standard/additional descriptors and implement explicit configuration, claim/release, and
   alternate-setting transitions with generation validation.
3. Add Java immutable values, `AutoCloseable` wrappers, Looper guard, and thin JNI marshalling.

### Phase 3 — Cancellable transfer primitives

1. Add async backend control/bulk/interrupt submissions behind synchronous public calls.
2. Validate direction/type, endpoint generation, buffer slice, 65,535-byte/1-MiB limits, and
   1–60,000-ms timeouts before JNI/native narrowing.
3. Serialize per-session work, cancel active transfer on close, enforce the 2-second bound, and test
   partial completion, stall, timeout, cancellation, detach, and multi-session isolation.

### Phase 4 — Stable publication and adapter proof

1. Publish headers/classes/symbols in the existing AAR/Prefab artifact and compile previous-baseline
   Java, C, and C++ consumers.
2. Route STLINK open/claim/transfer/close through the internal transport while keeping existing API,
   ownership, and mutation denials unchanged.
3. Document generic capability versus verified adapter support and record only sanitized,
   non-destructive hardware evidence.

### Phase 5 — GitHub task delivery

1. Regenerate `tasks.md`, then create exactly one issue per task with task ID, dependencies,
   acceptance checks, and links back to this feature.
2. For each ready issue, create `feat/<issue>-<task>-<slug>` from the latest `dev`, implement only that
   task, use the mandated author/committer, push, and open a PR targeting `dev` with `Closes #<issue>`.
3. Update the issue at start, PR creation, local-test failure/retry, and completion. Merge only when
   the PR is current with `dev`, all required local tests and public-policy gates pass with recorded
   evidence, and no unresolved review remains; then delete the feature branch and confirm closure.
4. Independent `[P]` tasks may overlap only when their file scopes do not conflict. Dependent tasks
   are created as issues immediately but branches start only after prerequisite PRs merge.

## GitHub Execution Gates

- Repository issues must remain enabled.
- The active GitHub identity must have at least `WRITE` permission; a read-only credential blocks all
  issue, PR, and merge mutations without blocking local planning.
- Every PR targets `dev`; `main` remains release-only.
- GitHub Actions are intentionally disabled and MUST NOT be enabled or dispatched without explicit
  maintainer approval. Existing workflow files remain for later reactivation.
- A PR cannot merge until the task's native, Android, publication, and policy checks pass locally and
  their exact commands/results are recorded in its issue and PR.
- One task maps to one issue and one PR; task splitting creates new issue IDs before new work begins.
- Public commit metadata must be `Marci Kopa <marcin@marcin.info>` for both author and committer.
- Tracked content, branch names, issue/PR text, logs, and evidence must contain no private branding,
  local paths, sensitive device identifiers, or unsupported hardware claims.

## Complexity Tracking

No constitution violation requires an exception. The shared event thread is justified by the
accepted 2-second cancellation requirement and multiple-session scale; per-session event threads
would consume more resources and synchronous libusb calls cannot provide the required cancellation
contract. The GitHub one-task/one-PR workflow adds delivery overhead by explicit user requirement,
not product architecture.

## Post-Design Validation (2026-08-12)

### Quickstart execution

- **PASS — source preparation**: recursive submodules initialized and the worktree contained no
  tracked generated binary.
- **PASS — deterministic native contracts**: the complete fake-backend CTest suite passed. An
  unavailable generator recorded in an old CMake cache was eliminated by the verifier's clean,
  explicitly selected Clang/Ninja build; the quickstart now uses that reproducible entry point.
- **PASS — managed and Android packaging**: unit tests, lint, release AAR assembly, publication
  verification, and the detached Android consumer build passed. The quickstart now exports the SDK
  resolved from `local.properties` when an invoking shell has no Android SDK environment variable.
- **PASS — compatibility and cleanliness**: publication verification found the expected 28 stable
  C ABI symbols in all six JNI/Prefab libraries, and `git diff --check` passed.
- **NOT RUN — optional hardware smoke**: no hardware result is required for generic transport; no
  connected device was opened, claimed, transferred, reset, written, erased, or otherwise changed.
- **PASS — GitHub readiness**: issues are enabled and the authenticated maintainer permission is
  `ADMIN`. Actions remain disabled and local evidence remains the merge gate.

### Constitution and compatibility result

**PASS**. The delivered implementation preserves the portable C++17 core and thin Android/JNI
adapters, application-owned `UsbDeviceConnection` lifecycle, worker-thread-only blocking calls,
fake-transport automation, bounded error and descriptor data, and non-destructive evidence policy.
It adds no runtime dependency and introduces no constitution exception.

Compatibility is additive: the managed baseline remains available, all 28 published C ABI symbols
are present for `arm64-v8a`, `armeabi-v7a`, and `x86_64`, Prefab headers and source/Javadoc artifacts
are consumable, and the existing read-only STLINK surface retains its signatures and denial of
mutating operations. No API or ABI break was detected. This feature is ready on `dev` for maintainer
testing; promotion to release-only `main` remains a separate decision.

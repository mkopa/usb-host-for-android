---

description: "Dependency-ordered implementation tasks for the public generic USB transport"
---

# Tasks: Generic USB Transport

**Input**: Design documents from `/specs/004-generic-usb-transport/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md, contracts/

**Tests**: Required. Every implementation slice writes its listed contract tests first, confirms the
intended failure locally, completes the implementation, and opens a PR only after the slice is green.

**Delivery**: Every checkbox task maps idempotently to one GitHub issue, one
`feat/<issue>-<task>-<slug>` branch from current `dev`, and one PR to `dev`. The issue is updated at
start, PR creation, local-test failure/retry, and merge. GitHub Actions remain disabled; the assistant
merges only after required local commands pass and their results are recorded in the issue and PR.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: May run in parallel after its stated prerequisites because it changes disjoint files.
- **[Story]**: Required only in user-story phases (`US1`, `US2`, `US3`).
- Every task names its exact primary file paths and must leave its branch buildable and testable.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish task delivery, compatibility baselines, and reusable verification entrypoints.

- [X] T001 Add public-safe task issue forms and the `dev` pull-request checklist in `.github/ISSUE_TEMPLATE/feature-task.yml` and `.github/pull_request_template.md`
- [X] T002 [P] Record existing C symbols, status values, and managed public classes in `native-tests/public-symbols-baseline.txt` and `usbHostForAndroid/src/test/resources/public-managed-api-baseline.txt`
- [X] T003 Add idempotent task-marker and one-task/one-issue validation to `scripts/verify-spec-task-issues.ps1`
- [X] T004 Add one-command host sanitizer and task/PR policy verification in `scripts/verify-local.ps1`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Freeze additive API/ABI primitives, portable values, errors, backend seam, handles, and
Android event runtime required by every user story.

**⚠️ CRITICAL**: No user-story branch starts until T001–T012 are merged into `dev`.

- [X] T005 Add failing-first numeric mapping tests and append stall, cancelled, and unsupported-operation statuses in `usbHostForAndroid/src/main/cpp/tests/status_contract_test.cpp`, `usbHostForAndroid/src/main/cpp/include/usbhost/usbhost.h`, and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/UsbTransportStatus.java`
- [X] T006 Add failing-first C/C++ compile contracts and define opaque handles, size-versioned records, limits, and additive function declarations in `usbHostForAndroid/src/main/cpp/tests/transport_header_contract_test.c`, `usbHostForAndroid/src/main/cpp/tests/transport_header_cxx_contract_test.cpp`, and `usbHostForAndroid/src/main/cpp/include/usbhost/transport.h`
- [X] T007 Add boundary-first tests and portable descriptor, request, result, generation, and session-state values in `usbHostForAndroid/src/main/cpp/tests/transport_types_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/types.hpp`
- [X] T008 [P] Add redaction/error mapping tests and bounded diagnostic sanitization in `usbHostForAndroid/src/main/cpp/tests/transport_error_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/error.hpp`, and `usbHostForAndroid/src/main/cpp/transport/error.cpp`
- [X] T009 Add scripted completion tests, the portable backend interface, and deterministic fake transport in `usbHostForAndroid/src/main/cpp/tests/scripted_usb_backend_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/backend.hpp`, and `usbHostForAndroid/src/main/cpp/tests/scripted_usb_backend.hpp`
- [X] T010 [P] Add stale-generation/concurrent-access tests and a generation-safe transport registry in `usbHostForAndroid/src/main/cpp/tests/transport_registry_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/registry.hpp`, and `usbHostForAndroid/src/main/cpp/transport/registry.cpp`
- [X] T011 Add lifecycle tests and the reference-counted no-discovery libusb context/event-thread runtime in `usbHostForAndroid/src/main/cpp/tests/libusb_runtime_contract_test.cpp`, `usbHostForAndroid/src/main/cpp/android/libusb_runtime.hpp`, and `usbHostForAndroid/src/main/cpp/android/libusb_runtime.cpp`
- [X] T012 [P] Add validation/value tests and immutable direction, transfer-type, result, exception, and control-request classes in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/TransportValueTypesTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/`

**Checkpoint**: The additive contracts compile, compatibility baselines are recorded, fake completions
are deterministic, and the Android runtime can be acquired/released without scanning USB devices.

---

## Phase 3: User Story 1 - Open an Authorized Generic USB Device (Priority: P1) 🎯 MVP

**Goal**: Open from an application-owned authorized connection, expose immutable standard/additional
descriptors, explicitly select configuration/alternate setting, claim/release interfaces, and close
without stealing the Android connection.

**Independent Test**: A scripted device with multiple configurations, alternate settings, endpoints,
and additional descriptors opens, enumerates, switches configuration, claims/releases, rejects stale
objects, closes twice, and proves the caller's connection ownership is unchanged.

### Test-first implementation slices for User Story 1

- [X] T013 [US1] Add malformed/overflow/duplicate descriptor tests and implement atomic standard descriptor snapshots in `usbHostForAndroid/src/main/cpp/tests/transport_descriptor_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/descriptors.hpp`, and `usbHostForAndroid/src/main/cpp/transport/descriptors.cpp`
- [X] T014 [US1] Add scoped additional-descriptor tests and implement bounded immutable `type + raw bytes` records in `usbHostForAndroid/src/main/cpp/tests/transport_additional_descriptor_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/descriptors.cpp`
- [X] T015 [US1] Add failed-open/idempotent-close/FD-ownership tests and implement portable session open/close state transitions in `usbHostForAndroid/src/main/cpp/tests/transport_session_lifecycle_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/session.hpp`, and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [X] T016 [US1] Add busy/rollback/stale-generation tests and implement explicit configuration selection with snapshot refresh in `usbHostForAndroid/src/main/cpp/tests/transport_configuration_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [X] T017 [US1] Add claim/release/alternate-setting state-machine tests and implement generation-bound interface/endpoint lifecycle in `usbHostForAndroid/src/main/cpp/tests/transport_interface_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [X] T018 [P] [US1] Add Android backend failure-cleanup tests and implement FD duplication, `libusb_wrap_sys_device`, descriptor extraction, and ordered cleanup in `usbHostForAndroid/src/main/cpp/tests/android_usb_backend_contract_test.cpp`, `usbHostForAndroid/src/main/cpp/android/android_usb_backend.hpp`, and `usbHostForAndroid/src/main/cpp/android/android_usb_backend.cpp`
- [X] T019 [US1] Add C ABI ownership/query/state contracts and implement open, descriptor enumeration, configuration, claim, alternate-setting, release, and close entrypoints in `usbHostForAndroid/src/main/cpp/tests/transport_c_api_session_test.c` and `usbHostForAndroid/src/main/cpp/transport/c_api.cpp`
- [X] T020 [P] [US1] Add equality/defensive-copy/unmodifiable-list tests and implement all immutable managed descriptor classes in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/DescriptorModelTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/`
- [X] T021 [US1] Add JNI shape/error tests and implement descriptor/session/interface marshalling without retained Android or JNI objects in `usbHostForAndroid/src/main/cpp/tests/transport_jni_contract_test.cpp`, `usbHostForAndroid/src/main/cpp/jni/transport_jni.cpp`, and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/TransportNativeBridge.java`
- [X] T022 [US1] Add worker-thread/caller-ownership/repeated-close tests and implement `GenericUsbDevice.open`, descriptor access, configuration selection, cancellation hook, and lifecycle in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/GenericUsbDeviceTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/GenericUsbDevice.java`
- [X] T023 [US1] Add foreign/stale/idempotent-release tests and implement claimed `GenericUsbInterface` alternate-setting lifecycle in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/GenericUsbInterfaceTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/GenericUsbInterface.java`
- [ ] T024 [US1] Add an isolated Maven-consumer descriptor/ownership fixture and publication assertions in `smoke-tests/android-consumer/src/main/java/info/marcin/usbhost/consumer/GenericDescriptorConsumer.java` and `usbHostForAndroid/build.gradle`

**Checkpoint**: User Story 1 is a separately usable descriptor/interface MVP. It does not yet claim
generic transfer or device-class support.

---

## Phase 4: User Story 2 - Exchange Control, Bulk, and Interrupt Data (Priority: P1)

**Goal**: Execute bounded synchronous public transfers over cancellable async libusb operations with
exact counts, stable errors, partial completion, multi-session isolation, and 2-second close.

**Independent Test**: Script control/bulk/interrupt IN/OUT, zero/short/boundary payloads, invalid
endpoints, timeout, stall, partial cancellation, disconnect, concurrent close, and two devices; all
complete deterministically with no leak, race, deadlock, or uninitialized-byte exposure.

### Test-first implementation slices for User Story 2

- [ ] T025 [P] [US2] Add overflow/direction/type/generation/timeout boundary tests and implement portable transfer validation in `usbHostForAndroid/src/main/cpp/tests/transport_validation_test.cpp`, `usbHostForAndroid/src/main/cpp/transport/transfer_validation.hpp`, and `usbHostForAndroid/src/main/cpp/transport/transfer_validation.cpp`
- [ ] T026 [US2] Add control IN/OUT/zero/short packet tests and implement synchronous session control dispatch over backend completions in `usbHostForAndroid/src/main/cpp/tests/transport_control_transfer_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [ ] T027 [US2] Add bulk/interrupt IN/OUT/short/boundary tests and implement claimed-endpoint dispatch in `usbHostForAndroid/src/main/cpp/tests/transport_endpoint_transfer_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [ ] T028 [US2] Add timeout/stall/disconnect/partial-count tests and implement stable completion/error propagation in `usbHostForAndroid/src/main/cpp/tests/transport_terminal_result_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [ ] T029 [US2] Add concurrent cancel/close/deadline tests and implement active-operation cancellation with the 2-second close bound in `usbHostForAndroid/src/main/cpp/tests/transport_cancellation_test.cpp` and `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [ ] T030 [US2] Add two-session isolation and same-session busy/serialization tests in `usbHostForAndroid/src/main/cpp/tests/transport_concurrency_test.cpp` and complete synchronization in `usbHostForAndroid/src/main/cpp/transport/session.cpp`
- [ ] T031 [P] [US2] Add libusb callback/status/cancellation tests and implement async control/bulk/interrupt submission on the shared event runtime in `usbHostForAndroid/src/main/cpp/tests/android_usb_transfer_contract_test.cpp` and `usbHostForAndroid/src/main/cpp/android/android_usb_backend.cpp`
- [ ] T032 [US2] Add C transfer/cancel/actual-count contracts and implement control, bulk, interrupt, and cancel C entrypoints in `usbHostForAndroid/src/main/cpp/tests/transport_c_api_transfer_test.c` and `usbHostForAndroid/src/main/cpp/transport/c_api.cpp`
- [ ] T033 [US2] Add JNI slice/partial-copy/exception tests and implement bounded caller-buffer marshalling in `usbHostForAndroid/src/main/cpp/tests/transport_jni_transfer_test.cpp` and `usbHostForAndroid/src/main/cpp/jni/transport_jni.cpp`
- [ ] T034 [US2] Add control request/result/main-thread tests and expose guarded control transfer and cancellation in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/GenericUsbControlTransferTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/GenericUsbDevice.java`
- [ ] T035 [US2] Add endpoint ownership/type/partial-result tests and expose guarded bulk/interrupt transfers in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/GenericUsbEndpointTransferTest.java` and `usbHostForAndroid/src/main/java/info/marcin/usbhost/transport/GenericUsbInterface.java`
- [ ] T036 [US2] Extend the detached Maven consumer with scripted transfer compile/use contracts in `smoke-tests/android-consumer/src/main/java/info/marcin/usbhost/consumer/GenericTransferConsumer.java` and `smoke-tests/android-consumer/src/test/java/info/marcin/usbhost/consumer/GenericTransferApiTest.java`

**Checkpoint**: User Stories 1 and 2 provide the full public generic synchronous transport with no
protocol-specific adapter or physical hardware requirement.

---

## Phase 5: User Story 3 - Build Protocol Adapters on One Stable Boundary (Priority: P2)

**Goal**: Prove Java/Kotlin and C/C++ adapter consumption, publish stable AAR/Prefab contracts, and
route existing safe STLINK-V3 I/O through the shared transport without API/ABI regression.

**Independent Test**: Managed and C/C++ adapter fixtures compile from the release-candidate artifact,
read an additional descriptor, perform a scripted transfer without private JNI, and all existing
STLINK read-only/mutation-denial tests pass through the shared runtime.

### Test-first implementation slices for User Story 3

- [ ] T037 [P] [US3] Add a managed protocol-adapter fixture that reads an additional descriptor and performs one scripted transfer using only public APIs in `usbHostForAndroid/src/test/java/info/marcin/usbhost/transport/ProtocolAdapterContractTest.java`
- [ ] T038 [P] [US3] Add exception-free C and C++ Prefab consumer fixtures covering all transport records and functions in `usbHostForAndroid/src/main/cpp/tests/transport_c_consumer_test.c` and `usbHostForAndroid/src/main/cpp/tests/transport_cxx_consumer_test.cpp`
- [ ] T039 [US3] Add previous-baseline enum/struct/symbol checks and extend additive export visibility in `usbHostForAndroid/src/main/cpp/tests/transport_compatibility_test.cpp`, `usbHostForAndroid/src/main/cpp/exports.map`, and `scripts/verify-publication.ps1`
- [ ] T040 [US3] Add adapter callback contracts and implement the internal STLINK-to-generic-transport bridge in `usbHostForAndroid/src/main/cpp/tests/stlink_transport_adapter_test.cpp`, `usbHostForAndroid/src/main/cpp/stlink/stlink_transport_adapter.hpp`, and `usbHostForAndroid/src/main/cpp/stlink/stlink_transport_adapter.cpp`
- [ ] T041 [US3] Add open/claim/transfer/close regression tests and replace direct STLINK libusb ownership with injected transport operations in `usbHostForAndroid/src/main/cpp/tests/stlink_usb_contract_test.cpp` and `usbHostForAndroid/src/main/cpp/stlink/stlink_usb_android.c`
- [ ] T042 [US3] Add caller-connection and native-FD ownership regression tests and wire STLINK sessions to the shared runtime in `usbHostForAndroid/src/main/cpp/tests/stlink_backend_transport_test.cpp` and `usbHostForAndroid/src/main/cpp/stlink/stlink_backend.cpp`
- [ ] T043 [US3] Extend read-only and mutation-denial regression coverage without adding mutating exports in `usbHostForAndroid/src/main/cpp/tests/c_api_contract_test.c` and `usbHostForAndroid/src/test/java/info/marcin/usbhost/StlinkSessionTest.java`
- [ ] T044 [US3] Verify transport classes, headers, symbols, source docs, and previous STLINK surface in local Maven publication from `usbHostForAndroid/build.gradle` and `scripts/verify-publication.ps1`
- [ ] T045 [P] [US3] Document a minimal Java/Kotlin adapter and C/C++/future-Rust consumption boundary in `docs/transport-adapter-example.md` and `docs/native-api.md`

**Checkpoint**: The artifact exposes a stable adapter foundation, and STLINK-V3 remains the only
adapter with its existing verified support claim and unchanged safety surface.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Complete public documentation, sanitized local validation, and final governance.

- [ ] T046 [P] Document setup, ownership, lifecycle, threading, limits, errors, explicit configuration, and capability non-claims in `docs/transport.md` and `README.md`
- [ ] T047 [P] Add the non-destructive generic USB smoke procedure and sanitized evidence fields in `docs/hardware/evidence-template.md` and `docs/hardware/README.md`
- [ ] T048 Run host CTest with sanitizer support, JUnit, lint, AAR assembly, detached consumer, and publication inspection through `scripts/verify-local.ps1`
- [ ] T049 Add final public-content, dependency, symbol, API/ABI, task-marker, and commit-identity audit coverage in `scripts/verify-publication.ps1` and `scripts/verify-spec-task-issues.ps1`
- [ ] T050 Execute every command and expected outcome from `specs/004-generic-usb-transport/quickstart.md`, then record the post-design constitution and compatibility result in `specs/004-generic-usb-transport/plan.md`

---

## Dependencies & Execution Order

### GitHub issue and PR envelope

1. Run `$speckit-taskstoissues` after this file is approved; the hidden feature/task marker prevents
   duplicate issues on repeated synchronization.
2. Create a branch only when all task prerequisites below are merged into `dev`.
3. Inside each task, write the stated tests first and observe the intended failure locally; finish the
   implementation before committing so its PR remains mergeable.
4. Push with mandated author/committer identity, open one PR to `dev` with `Closes #<issue>`, update
   the issue, record passing local tests and review/public-policy gates, merge, confirm issue closure,
   then start dependents.

### Phase dependencies

- **Setup (T001–T004)**: Starts immediately; T001/T002 may run concurrently, then T003 and T004.
- **Foundation (T005–T012)**: Depends on Setup and blocks all user stories.
- **US1 (T013–T024)**: Depends on Foundation and delivers the MVP descriptor/interface session.
- **US2 (T025–T036)**: Depends on US1 because transfers require active descriptors and claims.
- **US3 (T037–T045)**: Depends on US2 because adapter proofs and STLINK migration require transfers.
- **Polish (T046–T050)**: Depends on all selected user stories.

### Detailed prerequisite chain

```text
T001,T002 → T003 → T004
T005 → T006 → T007 → T009 → T011
             ├→ T008
             ├→ T010
             └→ T012
Foundation → T013 → T014 → T015 → T016 → T017 → T019 → T021 → T022 → T023 → T024
                         └→ T018 ────────────────┘
US1 → T025 → T026 → T027 → T028 → T029 → T030 → T032 → T033 → T034 → T035 → T036
       └→ T031 ────────────────────────────────┘
US2 → T037,T038 → T039 → T040 → T041 → T042 → T043 → T044
                                            └→ T045
US3 → T046,T047 → T048 → T049 → T050
```

### User story dependencies

- **US1**: Independently validates permission-safe open, owned descriptor snapshots, explicit
  configuration/interface lifecycle, and caller connection ownership.
- **US2**: Functionally depends on US1's session/endpoint state but is independently validated with a
  scripted backend and does not require STLINK or hardware.
- **US3**: Functionally depends on US2's transfer primitives and independently validates public
  adapter consumption, compatibility, and existing STLINK behavior.

### Parallel opportunities

- T001 and T002 touch delivery templates versus compatibility baselines.
- T008, T010, and T012 can proceed after their shared status/type prerequisites on disjoint files.
- T018 and T020 can proceed alongside native session-state work after descriptor contracts stabilize.
- T025 and T031 can proceed after US1 on portable validation versus Android backend files.
- T037 and T038 create independent managed and native adapter fixtures.
- T045, T046, and T047 modify separate documentation/evidence files after their contracts stabilize.

---

## Parallel Examples

### User Story 1

```text
Issue/branch A: T018 Android FD/libusb backend in android_usb_backend.*
Issue/branch B: T020 managed immutable descriptor model in info.marcin.usbhost.transport
```

### User Story 2

```text
Issue/branch A: T025 portable transfer validation and boundary tests
Issue/branch B: T031 Android asynchronous libusb submission and callback tests
```

### User Story 3

```text
Issue/branch A: T037 managed protocol-adapter fixture
Issue/branch B: T038 C/C++ Prefab consumer fixtures
```

---

## Implementation Strategy

### MVP first

1. Complete and merge Setup and Foundation task PRs.
2. Complete T013–T024 for User Story 1.
3. Stop and run the US1 independent test plus publication compile check.
4. Do not advertise generic transfer or class support at this checkpoint.

### Functional transport increment

1. Complete T025–T036 after US1 merges.
2. Run all fake control/bulk/interrupt/error/cancellation/concurrency contracts.
3. Validate the AAR consumer without physical hardware.
4. Advertise generic transport primitives, not class-specific support.

### Adapter-proven increment

1. Complete T037–T045 after US2 merges.
2. Compile Java/Kotlin and C/C++ consumers from the release-candidate artifact.
3. Run the unchanged safe STLINK regression suite through the shared transport.
4. Finish T046–T050 and record only sanitized, non-destructive hardware evidence.

---

## Notes

- Exactly 50 task IDs exist; task-to-issue synchronization must produce exactly 50 unique markers.
- `[P]` means the implementation branches may coexist, but each still rebases/updates from current
  `dev` before merge and cannot bypass its own dependencies.
- No task may add a runtime dependency without first updating research, plan, and its issue rationale.
- No task may add serial, DFU, CMSIS-DAP, HID, printer, analyzer, other programmer, or arbitrary
  desktop-libusb compatibility claims.
- Hardware validation is optional for generic transport, non-destructive, sanitized, and insufficient
  by itself to establish a new adapter support claim.
- GitHub Actions remain disabled until explicitly restored; workflow files stay intact and all merge
  evidence comes from local verification recorded in the task issue and PR.
- `main` remains release-only and is outside every task branch and PR in this feature.

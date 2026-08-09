# Tasks: ST-Link Android Host

**Input**: Design documents from `specs/001-stlink-android-host/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Required by FR-018 and the project constitution. Tests precede their implementation.

**Organization**: Tasks are grouped by independently testable user story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel because it changes different files and has no incomplete dependency
- **[Story]**: User story from spec.md

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Reproducible Android/native project and dependency baseline

- [ ] T001 Create Gradle 9.4.1 multi-module project files in `settings.gradle`, `build.gradle`,
  `gradle.properties`, and `gradle/wrapper/gradle-wrapper.properties`
- [ ] T002 [P] Pin libusb v1.0.30 and stlink revision as submodules in `.gitmodules` and document
  their licenses and revisions in `THIRD_PARTY_NOTICES.md`
- [ ] T003 [P] Create Android/C++/Windows ignore patterns in `.gitignore`
- [ ] T004 Configure Android library and sample modules in `usbHostForAndroid/build.gradle`,
  `usbHostExample/build.gradle`, and both `src/main/AndroidManifest.xml` files

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Stable contracts, portable abstractions, and test harness required by every story

**CRITICAL**: No user-story implementation begins until this phase completes.

- [ ] T005 Create stable status values, size-prefixed structures, ownership docs, and function
  declarations in `usbHostForAndroid/src/main/cpp/include/usbhost/usbhost.h`
- [ ] T006 [P] Create native backend abstraction, session states, and value types in
  `usbHostForAndroid/src/main/cpp/core/backend.hpp` and `core/types.hpp`
- [ ] T007 [P] Create Java status, exception, programmer, and target value types in
  `usbHostForAndroid/src/main/java/dev/usbhost/android/`
- [ ] T008 Configure host CTest and Android native targets in
  `usbHostForAndroid/src/main/cpp/CMakeLists.txt` and `native-tests/CMakeLists.txt`

**Checkpoint**: Public contracts and test infrastructure compile without hardware.

---

## Phase 3: User Story 1 - Discover and Open a Programmer (Priority: P1) MVP

**Goal**: Deterministically discover supported ST-Link V3 devices and safely open/close a
programmer-only session from an Android-authorized connection.

**Independent Test**: Fake-native lifecycle tests pass; Java device classification is deterministic;
on hardware, open reports programmer version without requiring a target.

### Tests for User Story 1

- [ ] T009 [P] [US1] Add ST-Link VID/PID and deterministic discovery unit tests in
  `usbHostForAndroid/src/test/java/dev/usbhost/android/StlinkUsbIdsTest.java` and
  `StlinkProberTest.java`
- [ ] T010 [P] [US1] Add fake-backend lifecycle tests and a wrapped-descriptor transport contract
  test in `usbHostForAndroid/src/main/cpp/tests/session_test.cpp` and
  `usbHostForAndroid/src/main/cpp/tests/stlink_usb_contract_test.cpp`

### Implementation for User Story 1

- [ ] T011 [US1] Implement mutex-protected session lifecycle and opaque handle registry in
  `usbHostForAndroid/src/main/cpp/core/session.cpp` and `core/registry.cpp`
- [ ] T012 [US1] Adapt pinned stlink USB backend to a duplicated, wrapped Android descriptor in
  `usbHostForAndroid/src/main/cpp/stlink/stlink_usb_android.c`
- [ ] T013 [US1] Implement programmer open/close and structured diagnostics in
  `usbHostForAndroid/src/main/cpp/stlink/stlink_backend.cpp` and `core/c_api.cpp`
- [ ] T014 [US1] Implement JNI open/close/value conversion in
  `usbHostForAndroid/src/main/cpp/jni/jni_bridge.cpp`
- [ ] T015 [US1] Implement device classification, probing, permission validation, worker-thread
  enforcement, and session ownership in `usbHostForAndroid/src/main/java/dev/usbhost/android/`

**Checkpoint**: US1 is independently usable without a connected STM32 target.

---

## Phase 4: User Story 2 - Identify the STM32 Target (Priority: P1)

**Goal**: Connect with non-resetting SWD hot-plug semantics and report STM32G0B0RET6 identity.

**Independent Test**: Fake backend returns/caches the expected target descriptor; hardware reports
chip ID `0x467`, flash/SRAM characteristics, and voltage.

### Tests for User Story 2

- [ ] T016 [P] [US2] Add target-connect, unsupported-target, no-target, and cached-result tests in
  `usbHostForAndroid/src/main/cpp/tests/session_test.cpp`
- [ ] T017 [P] [US2] Add Java target value/equality tests in
  `usbHostForAndroid/src/test/java/dev/usbhost/android/TargetInfoTest.java`

### Implementation for User Story 2

- [ ] T018 [US2] Embed the pinned STM32G0B/G0C chip descriptor in
  `usbHostForAndroid/src/main/cpp/stlink/stlink_chipid_android.c`
- [ ] T019 [US2] Implement hot-plug target connection, ID `0x467` enforcement, and target metadata
  mapping in `usbHostForAndroid/src/main/cpp/stlink/stlink_backend.cpp`
- [ ] T020 [US2] Expose target connection through the C ABI and JNI in
  `usbHostForAndroid/src/main/cpp/core/c_api.cpp` and `jni/jni_bridge.cpp`
- [ ] T021 [US2] Implement `connectTarget()` state and value mapping in
  `usbHostForAndroid/src/main/java/dev/usbhost/android/StlinkSession.java`

**Checkpoint**: US2 proves Android-to-ST-Link-to-STM32 communication without memory writes.

---

## Phase 5: User Story 3 - Read Target Memory Safely (Priority: P2)

**Goal**: Return exact bytes for valid flash/SRAM ranges and reject invalid requests before USB I/O.

**Independent Test**: Fake backend covers aligned/unaligned/chunked/boundary reads; hardware reads
1 byte, 4 bytes, 1 KiB, and 64 KiB consistently.

### Tests for User Story 3

- [ ] T022 [P] [US3] Add overflow, zero, maximum-size, region-boundary, unaligned, and chunked-read
  tests in `usbHostForAndroid/src/main/cpp/tests/session_test.cpp`
- [ ] T023 [P] [US3] Add Java argument/state/read-result tests in
  `usbHostForAndroid/src/test/java/dev/usbhost/android/StlinkSessionTest.java`

### Implementation for User Story 3

- [ ] T024 [US3] Implement pre-I/O memory-range validation and exact-byte semantics in
  `usbHostForAndroid/src/main/cpp/core/session.cpp`
- [ ] T025 [US3] Implement aligned, bounded stlink read chunking in
  `usbHostForAndroid/src/main/cpp/stlink/stlink_backend.cpp`
- [ ] T026 [US3] Expose caller-buffer reads through the C ABI and JNI in
  `usbHostForAndroid/src/main/cpp/core/c_api.cpp` and `jni/jni_bridge.cpp`
- [ ] T027 [US3] Implement bounded Java `readMemory` in
  `usbHostForAndroid/src/main/java/dev/usbhost/android/StlinkSession.java`

**Checkpoint**: US3 is read-only and returns exact requested bytes across alignment boundaries.

---

## Phase 6: User Story 4 - Stable Native Integration Boundary (Priority: P3)

**Goal**: Make the same versioned C contract consumable by C++, JNI, and a future Rust binding.

**Independent Test**: A C translation unit and a C++ translation unit compile/link against the
published header and exercise fake-backed lifecycle/results without Android types.

### Tests for User Story 4

- [ ] T028 [P] [US4] Add C and C++ ABI compile/link contract tests in
  `usbHostForAndroid/src/main/cpp/tests/c_api_contract_test.c` and `cxx_api_contract_test.cpp`

### Implementation for User Story 4

- [ ] T029 [US4] Export only the documented C symbols and configure Prefab header/library publishing
  in `usbHostForAndroid/src/main/cpp/CMakeLists.txt` and `usbHostForAndroid/build.gradle`
- [ ] T030 [US4] Add ABI versioning and Rust-bindgen guidance in `docs/native-api.md`

**Checkpoint**: Native consumers require neither JNI nor Android framework headers.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Example, compliance, documentation, and complete validation

- [ ] T031 [P] Implement a minimal Java discovery/open/identify/read sample in
  `usbHostExample/src/main/java/dev/usbhost/example/MainActivity.java` and
  `usbHostExample/src/main/res/values/strings.xml`
- [ ] T032 [P] Add a redacted hardware validation form in `docs/hardware/evidence-template.md`
- [ ] T033 [P] Document setup, permission flow, supported PIDs/target, native use, limitations, and
  safety boundary in `README.md`
- [ ] T034 Run `./gradlew.bat clean test assembleDebug`, host CTest, `git diff --check`, and the
  non-destructive steps in `specs/001-stlink-android-host/quickstart.md`; record unavailable hardware
  validation explicitly in `docs/hardware/README.md`

---

## Dependencies & Execution Order

### Phase dependencies

- Setup has no dependencies.
- Foundational depends on Setup and blocks all stories.
- US1 depends on Foundational.
- US2 depends on US1 because it requires a programmer session.
- US3 depends on US2 because reads require an identified target and memory map.
- US4 depends on the final C operations from US1-US3.
- Polish depends on all selected stories.

### Within each story

- Tests are written first and observed failing or non-compiling for the missing behavior.
- Portable core precedes transport/JNI adapters where both change.
- Native behavior precedes Java exposure.
- Each checkpoint is validated before the next story.

### Parallel opportunities

- T002 and T003 can run beside T001.
- T006 and T007 can run in parallel after T005.
- Each story's Java tests and native tests are parallelizable.
- Documentation and evidence templates can run in parallel after public contracts stabilize.

## Implementation Strategy

### MVP first

1. Complete Setup and Foundational.
2. Complete US1 and validate programmer open/close without a target.
3. Complete US2 for the first useful end-to-end target identity.
4. Add US3 read-only communication.
5. Freeze and validate the native boundary in US4.

### Safety rules

- Do not add a public write, erase, reset, halt, run, or register-write operation.
- Do not claim physical hardware validation without an evidence record.
- Do not log USB serials or target memory contents by default.
- Preserve immutable upstream dependency revisions; Android changes belong in project adapters.

---

description: "Implementation tasks for the public Android SDK release 0.1.0"
---

# Tasks: Public SDK Release 0.1.0

**Input**: Design documents from `/specs/003-sdk-release/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Required by the feature specification for namespace/linkage, publication, native core,
Android assembly, sample state/lifecycle, and release gates.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel because it touches different files and has no incomplete dependency.
- **[Story]**: User story traceability (`US1`, `US2`, `US3`).
- Every task names the exact primary file or directory it changes.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Record the release contract and prepare shared version/plugin configuration.

- [x] T001 Set version, coordinates, plugin versions, and Compose BOM constants in `gradle.properties`
- [x] T002 Configure the Maven publish and Compose compiler plugins in `build.gradle`
- [x] T003 [P] Add public branch/release policy and environment prerequisites in `RELEASING.md`
- [x] T004 [P] Add `.gitattributes` rules for deterministic text and generated archive inputs

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish the exact namespace and package/linkage boundary required by all stories.

- [x] T005 Move Java SDK sources to `usbHostForAndroid/src/main/java/info/marcin/usbhost/` and change package declarations
- [x] T006 Update JNI exports in `usbHostForAndroid/src/main/cpp/jni/jni_bridge.cpp` for `info.marcin.usbhost.NativeBridge`
- [x] T007 Update keep rules and Android namespace in `usbHostForAndroid/consumer-rules.pro` and `usbHostForAndroid/build.gradle`
- [x] T008 Move JVM tests to `usbHostForAndroid/src/test/java/info/marcin/usbhost/` and change package declarations
- [x] T009 Add a namespace/linkage regression test in `usbHostForAndroid/src/test/java/info/marcin/usbhost/NamespaceContractTest.java`
- [x] T010 Run library unit/native linkage checks and remove all old package references from `usbHostForAndroid/`

**Checkpoint**: Public namespace, managed API, JNI, tests, and keep rules agree exactly.

---

## Phase 3: User Story 1 - Integrate the Published Android Library (Priority: P1) 🎯 MVP

**Goal**: Produce a valid, signed-ready local 0.1.0 AAR publication under the documented Maven
coordinates without performing a live upload.

**Independent Test**: Publish to an isolated local Maven repository, inspect the AAR/POM/module/
sources/docs artifacts, and compile a detached consumer import under `info.marcin.usbhost`.

### Tests for User Story 1

- [x] T011 [P] [US1] Add local publication contract verification in `usbHostForAndroid/build.gradle`
- [x] T012 [P] [US1] Add a detached consumer smoke project in `smoke-tests/android-consumer/`

### Implementation for User Story 1

- [x] T013 [US1] Configure 0.1.0 AAR, sources, documentation, Prefab, and local candidate repository publication in `usbHostForAndroid/build.gradle`
- [x] T014 [US1] Configure complete Maven Central POM metadata and in-memory signing in `usbHostForAndroid/build.gradle`
- [x] T015 [US1] Configure Central Portal automatic publication using environment-backed credentials in `usbHostForAndroid/build.gradle`
- [x] T016 [US1] Add migration, Maven dependency, Kotlin usage, and release-candidate status to `README.md`
- [x] T017 [US1] Run local publication and detached consumer smoke checks from `smoke-tests/android-consumer/`

**Checkpoint**: User Story 1 is independently usable through the local repository and ready for
Central credentials after promotion.

---

## Phase 4: User Story 2 - Learn Through a Modern Kotlin Example (Priority: P2)

**Goal**: Deliver a polished read-only Kotlin/Compose programmer console for discovery, permission,
connection, target facts, and a 256-byte memory preview.

**Independent Test**: Assemble/install the sample, exercise no-device/permission/connect/read/detach
states, and audit that no mutation command exists.

### Tests for User Story 2

- [x] T018 [P] [US2] Add reducer and formatting tests in `usbHostExample/src/test/java/info/marcin/usbhost/example/ProgrammerUiStateTest.kt`
- [x] T019 [P] [US2] Add read-only command-surface audit in `usbHostExample/src/test/java/info/marcin/usbhost/example/ReadOnlySampleContractTest.kt`

### Implementation for User Story 2

- [x] T020 [US2] Replace Java activity and package with Kotlin entry point in `usbHostExample/src/main/java/info/marcin/usbhost/example/MainActivity.kt`
- [x] T021 [P] [US2] Implement immutable UI state and hex formatting in `usbHostExample/src/main/java/info/marcin/usbhost/example/ProgrammerUiState.kt`
- [x] T022 [US2] Implement lifecycle-safe USB permission/session controller in `usbHostExample/src/main/java/info/marcin/usbhost/example/ProgrammerController.kt`
- [x] T023 [US2] Implement Material 3 programmer console in `usbHostExample/src/main/java/info/marcin/usbhost/example/ProgrammerScreen.kt`
- [x] T024 [P] [US2] Add adaptive theme, colors, and strings in `usbHostExample/src/main/java/info/marcin/usbhost/example/ui/theme/` and `usbHostExample/src/main/res/`
- [x] T025 [US2] Configure built-in Kotlin, Compose, background execution, test dependencies, and namespace in `usbHostExample/build.gradle`
- [x] T026 [US2] Update explicit permission/detach manifest behavior in `usbHostExample/src/main/AndroidManifest.xml`
- [x] T027 [US2] Run sample unit tests, lint, debug/release assembly, and optional attached-device install

**Checkpoint**: User Stories 1 and 2 work independently; the sample presents no destructive target
operation.

---

## Phase 5: User Story 3 - Maintain and Promote a Trustworthy Release (Priority: P3)

**Goal**: Automate reproducible pull-request verification, controlled release publication, supply
chain checks, and a prebuilt Android runner while preserving the dev-first release gate.

**Independent Test**: Validate workflows, run the pinned container build, and execute a credentialless
release dry-run that proves publication remains blocked.

### Tests for User Story 3

- [x] T028 [P] [US3] Add workflow and release-policy validation script in `scripts/verify-release.ps1`
- [x] T029 [P] [US3] Add container toolchain smoke script in `docker/android-runner/smoke-test.sh`

### Implementation for User Story 3

- [x] T030 [US3] Add Gradle, Android, native, lint, publication, and artifact CI in `.github/workflows/ci.yml`
- [x] T031 [P] [US3] Add CodeQL and dependency-review workflows in `.github/workflows/codeql.yml` and `.github/workflows/dependency-review.yml`
- [x] T032 [P] [US3] Add Gradle/submodule/GitHub Actions update policy in `.github/dependabot.yml`
- [x] T033 [US3] Add main-contained tag checks, protected secrets, signing, Central upload, artifacts, and provenance in `.github/workflows/release.yml`
- [x] T034 [US3] Add pinned Android/NDK/CMake runner definition and usage docs in `docker/android-runner/Dockerfile` and `docker/android-runner/README.md`
- [x] T035 [US3] Add GHCR publication and optional Docker Hub mirror in `.github/workflows/runner-image.yml`
- [x] T036 [US3] Run workflow policy validation, Dockerfile build checks, and credentialless release gate tests

**Checkpoint**: All three user stories are independently functional and release automation fails
closed before any live publication.

---

## Phase 6: Polish & Dev Integration

**Purpose**: Complete documentation, evidence, public-policy review, and task-branch delivery to dev.

- [x] T037 [P] Reconcile `README.md`, `RELEASING.md`, `docs/native-api.md`, and `specs/003-sdk-release/quickstart.md`
- [x] T038 Run full Gradle tests/lint/assemblies, native CTest, local publication, consumer smoke, and namespace scans
- [x] T039 Inspect staged content for private branding, local paths, secrets, device identifiers, author, and committer metadata
- [x] T040 Commit with `Marci Kopa <marcin@marcin.info>`, push the task branch, and open a pull request to `dev`
- [x] T041 Merge the passing task pull request into `dev`, verify remote branch state, and stop before `dev` to `main`, `v0.1.0`, or Central publication

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: Starts immediately.
- **Foundational (Phase 2)**: Depends on T001-T002 and blocks all user stories.
- **US1 (Phase 3)**: Depends on the namespace/linkage foundation.
- **US2 (Phase 4)**: Depends on the namespace/linkage foundation; may proceed independently of live
  publication, but its build consumes the renamed local library.
- **US3 (Phase 5)**: Depends on the final build task names produced by US1 and US2.
- **Polish (Phase 6)**: Depends on all implemented user stories and all validation commands.

### User Story Dependencies

- **US1 (P1)**: No dependency on US2/US3 after Phase 2.
- **US2 (P2)**: Uses the US1 namespace contract but not the publication service.
- **US3 (P3)**: Encodes commands from US1/US2, so workflow completion follows both.

### Parallel Opportunities

- T003 and T004 can proceed together after shared configuration is known.
- T011 and T012 touch separate verification projects.
- T018 and T019 touch independent sample contract tests.
- T021 and T024 touch separate state and presentation assets after T020 establishes the package.
- T028/T029 and T031/T032 can proceed independently before workflow integration.
- T037 documentation reconciliation can start while long-running validation T038 executes.

## Implementation Strategy

### MVP First

1. Complete setup and the namespace/linkage foundation.
2. Complete US1 and prove a detached consumer can use the local 0.1.0 artifact.
3. Keep Central publication disabled while credentials/promotion are pending.

### Incremental Delivery

1. Add US2's Kotlin sample and validate read-only lifecycle behavior.
2. Add US3's CI/release/container controls and run dry-run failure cases.
3. Integrate the task PR to `dev`.
4. Stop for maintainer testing and explicit approval before any release promotion.

## Notes

- `[P]` denotes tasks safe to execute concurrently, not permission to skip dependencies.
- Test tasks are implemented before their paired production behavior where practical.
- No task in this list authorizes merging `dev` to `main`, tagging `v0.1.0`, or publishing live.
- The unrelated untracked file under `specs/001-stlink-android-host/` remains outside this feature.

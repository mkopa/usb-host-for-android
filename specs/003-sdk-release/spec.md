# Feature Specification: Public SDK Release 0.1.0

**Feature Branch**: `feature/sdk-release-0.1.0`

**Created**: 2026-08-09

**Status**: Draft

**Input**: Prepare the public Android USB host SDK release 0.1.0 with the exact
`info.marcin.usbhost` namespace, a modern Kotlin programmer-style example, automated public
artifact publication, comprehensive continuous integration, a reproducible container build
environment, and a development-first branching policy.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Integrate the Published Android Library (Priority: P1)

An Android developer adds version 0.1.0 from a standard public artifact repository, imports the
API from `info.marcin.usbhost`, and connects to a supported USB probe without copying source code or
building native dependencies manually.

**Why this priority**: A consumable, stable artifact and namespace are the release's core value.

**Independent Test**: Start an empty Android project, add the documented repository and dependency,
compile a minimal client that imports the public API, and verify the resolved artifact is 0.1.0.

**Acceptance Scenarios**:

1. **Given** a clean Android project with network access, **When** the developer adds the documented
   0.1.0 dependency, **Then** dependency resolution and compilation succeed without local modules.
2. **Given** client source importing `info.marcin.usbhost`, **When** the client compiles, **Then** all
   documented public API types resolve under that exact namespace.
3. **Given** a release prepared from the development branch, **When** no explicit promotion approval
   has been given, **Then** no release is published and the main branch remains unchanged.

---

### User Story 2 - Learn Through a Modern Kotlin Example (Priority: P2)

An Android developer runs a polished Kotlin sample that discovers compatible USB probes, requests
USB permission, connects, displays probe and target information, performs a bounded read-only memory
operation, and presents useful status and errors.

**Why this priority**: A working example is the shortest path from artifact adoption to a successful
hardware session and demonstrates safe lifecycle handling.

**Independent Test**: Install the sample on a supported Android device, connect and disconnect a
probe through USB OTG, grant or deny permission, and verify that the app remains responsive and does
not offer target write, erase, reset, halt, run, or step commands.

**Acceptance Scenarios**:

1. **Given** no attached probe, **When** the sample starts, **Then** it displays an actionable empty
   state and permits rescanning.
2. **Given** a compatible attached probe without permission, **When** the user selects it, **Then**
   the sample requests permission and reports approval or denial without blocking the interface.
3. **Given** an authorized connected probe, **When** the user requests target information or a
   bounded memory preview, **Then** the result and diagnostic context are shown safely.
4. **Given** a disconnection during an operation, **When** the transport fails, **Then** the session
   closes, stale data is marked, and the user can reconnect without restarting the app.

---

### User Story 3 - Maintain and Promote a Trustworthy Release (Priority: P3)

A maintainer develops on a task branch created from `dev`, merges reviewed work back to `dev`, runs
the same reproducible checks locally and in automation, and promotes a verified release to `main`
only after explicit approval.

**Why this priority**: Repeatable checks, provenance, and controlled promotion make the public
artifact supportable without slowing normal feature work.

**Independent Test**: Open a task change against `dev`, observe all required checks, build in the
documented container, and dry-run the release workflow without publishing or changing `main`.

**Acceptance Scenarios**:

1. **Given** a task branch based on `dev`, **When** a pull request is opened to `dev`, **Then** source,
   native, Android, documentation, and dependency checks run automatically.
2. **Given** the repository and a supported container runtime, **When** the documented container
   command is run, **Then** it uses a pinned toolchain and produces the same build outputs as CI.
3. **Given** approved and passing code on `dev`, **When** a maintainer promotes it to `main` and
   creates the documented release version, **Then** signed artifacts and release metadata are
   published automatically.
4. **Given** missing credentials, an incorrect version, a non-main release commit, or a failed
   verification, **When** release automation runs, **Then** it stops before publication.

### Edge Cases

- USB permission is denied, dismissed, or revoked while the sample is active.
- Multiple compatible probes are attached and one disconnects during enumeration.
- The target is absent or powered separately while the probe remains connected.
- A client still imports the previous development namespace after upgrading.
- Artifact signing or repository credentials are unavailable or malformed.
- A release tag/version already exists, or the artifact repository rejects duplicate 0.1.0 bytes.
- CI runs in a fork where publication secrets are intentionally unavailable.
- A submodule is missing or checked out at an unexpected revision.
- The container cache is empty, unavailable, or built on a different host architecture.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Android library MUST use the exact namespace `info.marcin.usbhost` for its public
  API and generated Android resources.
- **FR-002**: Release metadata MUST identify the artifact as version `0.1.0` and expose stable,
  documented public coordinates.
- **FR-003**: The 0.1.0 artifact MUST be consumable from a standard public repository supported by
  Android and Gradle clients without cloning this repository.
- **FR-004**: Published artifacts MUST include the binary library, source archive, API documentation
  archive, checksums, signatures, license, project URL, source-control coordinates, developer
  identity, and dependency metadata expected by the selected repository.
- **FR-005**: Publication MUST occur only through an authenticated release workflow after all required
  checks pass, and MUST fail closed when credentials or signing material are missing.
- **FR-006**: The project MUST document that task branches originate from and merge into `dev`, while
  public releases are promoted through `main` after explicit approval.
- **FR-007**: Automation MUST verify managed-language tests, native tests, Android library assembly,
  Kotlin sample assembly, static analysis, dependency changes, and public-release consistency.
- **FR-008**: The repository MUST provide a pinned, reproducible container build environment and a
  documented way to use it locally and in automation.
- **FR-009**: The sample application MUST be implemented in modern Kotlin and demonstrate discovery,
  permission handling, connection lifecycle, probe/target information, and a bounded read-only
  memory preview.
- **FR-010**: The sample MUST keep USB and native work off the Android main thread and expose progress,
  cancellation-safe lifecycle behavior, and actionable errors.
- **FR-011**: The sample MUST NOT expose target write, erase, reset, halt, run, step, option-byte,
  flash-programming, or peripheral-mutating commands in version 0.1.0.
- **FR-012**: Documentation MUST include complete clone, build, dependency, sample, container, branch,
  and release instructions using public, copyable values rather than placeholders.
- **FR-013**: Every new commit MUST use `Marci Kopa <marcin@marcin.info>` as both author and committer,
  and new tracked content MUST contain no private organization, customer, or product branding.
- **FR-014**: Release promotion from `dev` to `main`, creation of the 0.1.0 release tag, and live
  artifact publication MUST remain pending until the maintainer explicitly approves promotion.
- **FR-015**: Native entry points, keep rules, tests, and sample imports MUST remain consistent with
  the required namespace so runtime linkage does not depend on the previous package name.
- **FR-016**: The previous development namespace MUST not be advertised as part of the 0.1.0 API;
  migration guidance MUST identify the required replacement import.

### Key Entities

- **Library Release**: An immutable version with coordinates, namespace, binaries, source and
  documentation archives, signatures, checksums, provenance, and publication state.
- **Build Verification**: A named automated check with inputs, pinned environment, output evidence,
  pass/fail result, and release-gating role.
- **Probe Session**: The sample's lifecycle-bound relationship among Android USB permission, an open
  device connection, the native transport, target information, and latest read-only result.
- **Release Promotion**: The reviewed transition from `dev` through `main` to a public version, with
  explicit approval, verified commit identity, and publication outcome.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A clean consumer project resolves 0.1.0 and compiles an
  `info.marcin.usbhost` import in one documented setup attempt.
- **SC-002**: All required automated checks pass on the release candidate in both hosted automation
  and the documented container environment.
- **SC-003**: A developer can install the sample, authorize a connected compatible probe, view target
  information, and perform a bounded read-only preview in under five minutes using the README alone.
- **SC-004**: Namespace inspection finds zero public API, JNI entry-point, test, or sample references
  to the previous development package.
- **SC-005**: A release dry run proves that missing secrets, wrong branch provenance, duplicate
  version, or failed checks prevent publication in every tested case.
- **SC-006**: The release candidate reaches `dev` with no uncommitted implementation files, while
  `main` and public artifact repositories remain unchanged until explicit maintainer approval.
- **SC-007**: The first uncached container build completes without interactive setup and subsequent
  cached CI builds restore their dependency/toolchain cache automatically.

## Assumptions

- Version 0.1.0 is the first public compatibility baseline; compatibility with unpublished package
  names is not required.
- Maven Central is the intended primary public artifact repository because the deliverable is an
  Android library rather than a Google-owned SDK or a Gradle plugin; the implementation plan will
  validate this choice against current repository requirements.
- Repository and signing accounts may require maintainer-side enrollment or secrets; configuration
  can be completed and dry-run without exposing credentials in tracked files.
- Hosted automation uses standard public GitHub runners, while the container is optimized for local
  parity and optional prebuilt publication to a container registry.
- The sample is an integration demonstration, not a full production flash programmer, and version
  0.1.0 remains deliberately read-only at the target-operation level.
- Existing supported probe functionality and native dependencies remain the transport foundation;
  this feature packages and demonstrates them rather than expanding the supported hardware matrix.

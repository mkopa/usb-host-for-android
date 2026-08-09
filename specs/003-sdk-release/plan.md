# Implementation Plan: Public SDK Release 0.1.0

**Branch**: `feature/sdk-release-0.1.0` | **Date**: 2026-08-09 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/003-sdk-release/spec.md`

## Summary

Promote the existing Android/STLINK-V3 implementation into a consumable 0.1.0 SDK by migrating
every Java/JNI/package boundary to `info.marcin.usbhost`, publishing the release AAR and Prefab
package as `info.marcin.usbhost:usb-host-for-android:0.1.0`, replacing the Java sample with a
read-only Kotlin/Compose programmer console, and adding release-gated CI, Central Portal publishing,
provenance, and a pinned Android build container. Work is integrated into `dev`; `main`, the tag, and
live Central publication remain unchanged until explicit maintainer approval.

## Technical Context

**Language/Version**: Java 17, built-in Kotlin 2.3.21 through AGP 9.2.1, C17, C++17, CMake 3.22.1

**Primary Dependencies**: Android Gradle Plugin 9.2.1, Gradle 9.4.1, AndroidX Compose BOM
2026.06.01, Vanniktech Maven Publish Plugin 0.37.0, libusb 1.0.30, pinned stlink submodule

**Storage**: No persistent application storage; release artifacts and CI caches only

**Testing**: JUnit 4 host unit tests, CTest native contracts, Android Lint, Gradle publication
validation, sample APK assembly, optional physical USB smoke test

**Target Platform**: Android API 23+ for consumers; Linux GitHub runners and OCI-compatible local
container builds; Maven Central consumers

**Project Type**: Android AAR/native Prefab library plus Android sample application

**Performance Goals**: UI stays responsive during USB operations; cached CI avoids repeated SDK/NDK
downloads; bounded reads retain existing 1 MiB API ceiling while the sample previews only 256 bytes

**Constraints**: Exact namespace `info.marcin.usbhost`; target operations exposed by the sample are
read-only; no secrets in tracked files; release only from `main`; commit identity is fixed by policy

**Scale/Scope**: One AAR coordinate, one Compose sample screen with lifecycle/state components,
four primary workflows, one container definition, namespace migration across nine API classes and JNI

## Constitution Check

*GATE: Passed before Phase 0 research and re-checked after Phase 1 design.*

- **Portable core, thin adapters — PASS**: Namespace and UI work changes only Android/JNI adapters;
  C/C++ session logic and its Rust-ready C ABI remain unchanged.
- **Explicit stable boundaries — PASS**: The contract fixes Maven coordinates, namespace, JNI names,
  file-descriptor ownership, background execution, release provenance, and failure behavior.
- **Hardware-safe testing — PASS**: Automated checks remain fake-transport/read-only; the example does
  not expose write, erase, reset, halt, run, step, or register mutation.
- **Minimal reviewable delivery — PASS**: Existing modules are retained; one publishing plugin and
  Compose dependencies are justified by direct release and sample requirements.
- **Licensing/provenance — PASS**: Existing notices remain authoritative; publication metadata carries
  the MIT project license and source coordinates, while third-party binaries retain current notices.
- **Release governance — PASS**: Task work targets `dev`; promotion/tag/publication are deliberately
  excluded until explicit approval.

## Project Structure

### Documentation (this feature)

```text
specs/003-sdk-release/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── maven-publication.md
│   ├── release-promotion.md
│   └── sample-session.md
└── tasks.md
```

### Source Code (repository root)

```text
.github/
├── dependabot.yml
└── workflows/
    ├── ci.yml
    ├── codeql.yml
    ├── dependency-review.yml
    ├── release.yml
    └── runner-image.yml

docker/android-runner/
├── Dockerfile
└── README.md

usbHostForAndroid/
├── build.gradle
├── consumer-rules.pro
└── src/
    ├── main/java/info/marcin/usbhost/
    ├── main/cpp/{core,include,jni,stlink}/
    └── test/java/info/marcin/usbhost/

usbHostExample/
├── build.gradle
└── src/main/
    ├── java/info/marcin/usbhost/example/
    └── res/

native-tests/
docs/
README.md
RELEASING.md
```

**Structure Decision**: Keep the existing library and sample modules. Move managed source paths to
match the required namespace, preserve the native core/Prefab layout, and place delivery automation
at repository root so consumers do not inherit build infrastructure.

## Implementation Phases

### Phase 1 — Namespace and publication contract

1. Migrate Java package declarations, directories, tests, keep rules, JNI export symbols, and sample
   imports to `info.marcin.usbhost`.
2. Configure immutable 0.1.0 coordinates, sources and documentation artifacts, full POM metadata,
   in-memory signing, and Central Portal publication with Vanniktech 0.37.0.
3. Add a publication verification task that inspects POM/module/AAR/Prefab contents without network
   publication and fails on snapshot versions or old package references.

### Phase 2 — Kotlin programmer console

1. Replace the Java activity with a Kotlin/Compose state-driven screen using AGP's built-in Kotlin.
2. Implement USB discovery and permission events in a lifecycle-bound controller; use coroutines or
   a dedicated executor for open/connect/read and close sessions deterministically.
3. Present device selection, connection state, target/probe cards, a 256-byte formatted read-only
   preview, operation log, accessible states, and dark/light Material styling.

### Phase 3 — CI, release, and container

1. Add CI for submodules, Gradle validation/tests/lint/assemblies, native contracts, publication
   dry-run, and uploaded reports/artifacts.
2. Add CodeQL, dependency review, Dependabot, least-privilege permissions, concurrency cancellation,
   wrapper validation, dependency submission, and release provenance.
3. Add a tag/release workflow that verifies `v0.1.0`, commit containment in `main`, clean coordinates,
   credentials, signatures, tests, and Central publication before creating GitHub release artifacts.
4. Add a pinned Linux Android runner image and cache-aware build workflow. Publish to GHCR by default
   with `GITHUB_TOKEN`; optionally mirror to Docker Hub only when repository secrets are configured.

### Phase 4 — Documentation and promotion readiness

1. Update README dependency instructions, Kotlin usage, branch policy, container commands, and release
   status; add detailed maintainer release instructions.
2. Run all local non-hardware checks and inspect generated artifacts and namespace/linkage.
3. Commit with the mandated author/committer, push the task branch, review/merge to `dev`, and stop
   before any `dev` to `main` promotion, tag, or live publication.

## Complexity Tracking

No constitution violations require an exception.

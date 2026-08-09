<!--
Sync Impact Report
- Version change: template -> 1.0.0
- Added principles:
  - I. Portable Core, Thin Platform Adapters
  - II. Explicit and Stable Boundaries
  - III. Hardware-Safe, Evidence-Based Testing
  - IV. Minimal, Reviewable Delivery
- Added sections:
  - Technical Constraints
  - Development Workflow
- Removed sections: none (template placeholders replaced)
- Deferred items: none
-->
# USB Host for Android Constitution

## Core Principles

### I. Portable Core, Thin Platform Adapters
Protocol and device logic MUST live in a platform-neutral native core. Android-specific code,
including USB permission handling, file-descriptor ownership, lifecycle integration, and JNI,
MUST remain in explicit adapters. Public native interfaces MUST be usable from C++ without
Android framework types and MUST preserve a practical path to a future Rust implementation.

### II. Explicit and Stable Boundaries
Every boundary between Kotlin/Java, C++, third-party native libraries, and USB transports MUST
define ownership, lifetimes, threading, cancellation, error mapping, and compatibility rules.
Raw pointers, file descriptors, and native exceptions MUST NOT cross a language boundary without
an explicit ownership contract. Breaking API or ABI changes MUST be documented and versioned.

### III. Hardware-Safe, Evidence-Based Testing
Pure protocol and state-machine behavior MUST be covered by deterministic automated tests without
physical hardware. USB transport integrations MUST have contract tests. Claims of device support
MUST be backed by recorded tests on the named hardware and Android environment; destructive
target operations MUST require explicit intent and MUST fail safely on disconnect, timeout, or
cancellation.

### IV. Minimal, Reviewable Delivery
Work MUST proceed from an approved specification through plan, tasks, implementation, and
verification. Each increment MUST be the smallest coherent vertical slice, with new dependencies
and abstractions justified by a current requirement. Existing implementations MAY be used as
references, but code MUST NOT be copied without verifying its design, license, and suitability.

## Technical Constraints

- Android USB Host APIs are the authority for device permission and connection lifecycle.
- Native dependencies MUST have documented versions, licenses, provenance, and Android build
  compatibility.
- Blocking USB or native work MUST NOT run on the Android main thread.
- Errors MUST retain actionable native and USB context while exposing a stable public error model.
- Sensitive or device-identifying data MUST NOT be logged by default.

## Development Workflow

Each feature MUST follow
`specify -> clarify (when needed) -> plan -> tasks -> implement -> analyze`.
Plans MUST identify language and platform boundaries, dependency and licensing implications, test
levels, supported hardware, and recovery behavior. A change is complete only when relevant automated
checks pass, hardware-dependent claims have recorded evidence, and public behavior is documented.

## Governance

This constitution governs project specifications, plans, tasks, implementation, and review. An
amendment requires a documented rationale, an impact review for active specifications, and a
semantic version update: MAJOR for incompatible principle changes, MINOR for new or materially
expanded rules, and PATCH for clarifications. Every plan and review MUST verify compliance; any
exception MUST be explicit, narrowly scoped, justified, and time-bounded.

**Version**: 1.0.0 | **Ratified**: 2026-08-08 | **Last Amended**: 2026-08-08

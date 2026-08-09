# Feature Specification: STM32G0 Realtime Viewer

**Feature Branch**: `001-stlink-android-host` (shared worktree; dedicated branch deferred)

**Created**: 2026-08-08

**Status**: Draft

**Input**: Public demonstration viewer for STM32G0B0RET6 peripheral configuration, presented in a
human-readable form and restricted to non-destructive observation.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Observe a Demonstration Target Live (Priority: P1)

An engineer connects an authorized STM32G0B0RET6 demonstration target through ST-Link and
immediately sees a coherent live overview of board state, target identity, connection health,
update rate, and stale-data status.
The board continues running and the viewer never changes its memory or execution state.

**Why this priority**: A trustworthy, non-intrusive live view is the core user value and the safety
foundation for every deeper inspection feature.

**Independent Test**: Connect a running demonstration target, observe the dashboard for 30 minutes,
and verify that values update, stale or disconnected data is marked promptly, and an independent
target audit finds no write, erase, reset, halt, run, or step request.

**Acceptance Scenarios**:

1. **Given** a supported powered demonstration target and an authorized programmer, **When** the
   user starts live observation, **Then** the viewer shows target identity, supply voltage,
   freshness, sample rate, and the latest coherent project snapshot within three seconds.
2. **Given** a live session, **When** the programmer or target is disconnected, **Then** the viewer
   marks all affected values stale within one second and retains the last values only as historical
   context.
3. **Given** a running target, **When** the viewer observes it for 30 minutes, **Then** the target
   remains running and no destructive or state-changing debug operation is issued.

---

### User Story 2 - Understand Peripheral Configuration (Priority: P1)

An engineer selects a board subsystem or MCU peripheral and sees current settings translated into
human-readable meaning: enabled state, clock source and effective rate, pin modes and alternate
functions, channel configuration, DMA routing, interrupt enables, and important warnings. Raw
address, register value, and bit positions remain available as supporting evidence.

**Why this priority**: Register hex values alone do not answer whether the board is configured as
intended. Human interpretation is what makes the tool useful for commissioning and debugging.

**Independent Test**: Load a known demonstration-board configuration fixture and verify that
representative clock, GPIO, timer, ADC, DMA, UART, SPI, and I2C settings are described correctly and
traceable to their source register fields.

**Acceptance Scenarios**:

1. **Given** a known peripheral configuration, **When** the user opens its details, **Then** every
   supported field displays a name, decoded value, short explanation, raw evidence, and freshness.
2. **Given** a contradictory or incomplete configuration, **When** it is decoded, **Then** the
   viewer highlights the inconsistency and explains which evidence caused the warning.
3. **Given** a field without a project-specific decoder, **When** it is displayed, **Then** the
   viewer uses a generic numeric or bit-field representation and clearly labels it as undecoded.

---

### User Story 3 - Diagnose Changes Over Time (Priority: P2)

An engineer can freeze the live view, compare two coherent samples, filter changed values, and
inspect a bounded recent history without exposing raw target contents or device identifiers by
default.

**Why this priority**: Transient configuration and state changes are difficult to diagnose from a
single snapshot.

**Independent Test**: Replay a deterministic sequence of snapshots and verify change highlighting,
freeze/resume behavior, ordering, bounded history, and redacted diagnostic export.

**Acceptance Scenarios**:

1. **Given** changing live values, **When** the user enables "changed only", **Then** the viewer
   shows which human-readable values changed, their previous/current values, and timestamps.
2. **Given** a frozen view, **When** new samples arrive, **Then** the selected sample remains stable
   while the viewer indicates that newer data is available.
3. **Given** an export request, **When** diagnostics are created, **Then** serial numbers and raw
   target-memory blocks are excluded unless the user explicitly opts in.

---

### User Story 4 - Inspect Raw Registers Safely (Priority: P3)

An expert searches the STM32G0B0 register catalog by peripheral, register, field, or address and may
read registers classified as safe. Registers that are write-only, unknown, destructive-on-read, or
otherwise unsafe are blocked or shown only from the target-owned snapshot.

**Why this priority**: Expert access is valuable, but broad raw polling must not compromise running
firmware or bypass the safety boundary.

**Independent Test**: Use a catalog containing safe, write-only, read-side-effect, and unknown
registers; verify that only explicitly safe reads reach the transport and every blocked item gives a
clear reason.

**Acceptance Scenarios**:

1. **Given** the STM32G0B0 register catalog, **When** the user searches for a symbol or address,
   **Then** matching peripheral, register, and field definitions appear with access and safety
   classification.
2. **Given** a register not classified as safe for direct observation, **When** a read is requested,
   **Then** no target access occurs and the viewer explains the restriction.
3. **Given** a register approved for live polling, **When** repeated reads are enabled, **Then** the
   viewer applies a bounded rate and visibly reports the actual sample age.

### Edge Cases

- The target snapshot changes while it is being read; the viewer must reject the torn sample and
  retry without presenting mixed values.
- The connected firmware publishes an older, newer, corrupt, or unknown snapshot schema.
- A map register has duplicate aliases, overlapping fields, missing access metadata, or a field
  wider than its register.
- A peripheral clock is disabled, so its registers may be inaccessible or contain retained values.
- Read bandwidth is insufficient for the requested dashboard rate.
- The target resets, detaches, loses power, or changes firmware during observation.
- Multiple ST-Link devices or demonstration targets are attached.
- The user switches applications or locks the phone while a session is active.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST discover and open an explicitly selected supported demonstration
  target through the existing read-only programmer session and MUST never auto-select ambiguously
  among multiple targets.
- **FR-002**: The system MUST present target identity, firmware snapshot compatibility, measured
  target voltage, connection state, sample freshness, and effective update rate.
- **FR-003**: The system MUST consume only coherent, versioned project snapshots and MUST reject
  incomplete, corrupt, incompatible, or internally inconsistent samples.
- **FR-004**: The system MUST keep all target access read-only and MUST prevent memory writes,
  register writes, erase, option-byte changes, reset, halt, run, and step operations independently
  of user-interface state.
- **FR-005**: The system MUST stop or suspend live observation when target identity changes,
  permission is lost, the programmer detaches, or the application lifecycle no longer permits safe
  background work.
- **FR-006**: The system MUST decode project snapshot values into stable human-readable names,
  units, enum labels, boolean states, and concise explanations.
- **FR-007**: The system MUST decode at least the clock tree, GPIO, timers, ADC, DMA, UART, SPI, and
  I2C configuration used by the current demonstration firmware, with a generic evidence view for
  other mapped peripherals.
- **FR-008**: Every decoded value MUST retain traceability to its source sample, address or symbol,
  raw numeric evidence, decoder version, and timestamp.
- **FR-009**: The system MUST distinguish configuration, live state, counters, warnings, unknown
  values, stale values, and values unavailable because a peripheral is disabled.
- **FR-010**: The system MUST support search and filtering by board subsystem, peripheral, register,
  field, address, access type, changed state, warning state, and freshness.
- **FR-011**: The system MUST support freeze, resume, sample comparison, and a bounded recent
  history without allowing historical values to be confused with live values.
- **FR-012**: The system MUST load the STM32G0B0 catalog with its provenance and MUST validate the
  expected 44 peripherals, 625 registers, and 3885 fields before treating the supplied map as
  compatible.
- **FR-013**: Direct peripheral-register reads MUST use an explicit safety classification. Unknown,
  write-only, read-side-effect, destructive, or unclassified registers MUST NOT be read directly.
- **FR-014**: Live polling MUST be opt-in per approved group, rate-limited, cancellable, and
  automatically reduced when transport capacity or application lifecycle requires it.
- **FR-015**: Errors MUST identify the affected layer and recovery action without exposing USB
  serials or raw memory contents by default.
- **FR-016**: Diagnostic export MUST be redacted by default, bounded in size, and include enough
  schema, map, firmware, and timing metadata to reproduce the interpretation.
- **FR-017**: The same recorded snapshot fixtures and decoding rules MUST be usable without physical
  hardware for deterministic tests and future non-Android consumers.
- **FR-018**: The system MUST provide an explicit unsupported state rather than guessing whenever a
  firmware snapshot, map entry, decoder, or target identity is not recognized.

### Key Entities

- **Observation Session**: One authorized, read-only connection to a selected programmer and target,
  including lifecycle, identity, sampling state, and terminal error.
- **Project Snapshot**: A coherent, versioned set of demonstration-target state and configuration
  values published by the running target for observation.
- **Register Catalog**: Provenance-aware definitions of peripherals, registers, fields, access, and
  direct-read safety classification.
- **Decoder Rule**: A versioned interpretation that converts source evidence into a human-readable
  value, unit, label, explanation, and warning state.
- **Observed Value**: A decoded value with raw evidence, timestamp, freshness, source, quality, and
  optional previous value.
- **Sample History**: A bounded ordered sequence of coherent samples and change summaries.
- **Diagnostic Bundle**: A redacted export of identities, schema versions, timing, decoded values,
  warnings, and errors without raw memory blocks by default.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: An engineer sees the first coherent demonstration dashboard within three seconds of granting
  access in at least 95% of normal connection attempts.
- **SC-002**: The viewer presents at least five coherent live updates per second for the default
  dashboard for 30 minutes, with at least 99% of requested samples either displayed or explicitly
  accounted for as skipped, stale, or invalid.
- **SC-003**: Disconnection, target replacement, or stale data is visibly reported within one second
  and no stale value is presented as current.
- **SC-004**: All supported demonstration-board clock, GPIO, timer, ADC, DMA, UART, SPI, and I2C
  fixture settings are decoded correctly and remain traceable to raw evidence in automated
  acceptance tests.
- **SC-005**: One hundred percent of direct register-read attempts outside the approved safe catalog
  are blocked before target access.
- **SC-006**: A 30-minute hardware observation produces zero target write, erase, reset, halt, run,
  or step requests and does not interrupt the running target application.
- **SC-007**: A user can find a named peripheral setting and understand its current value, freshness,
  and source in under 30 seconds without consulting a reference manual.
- **SC-008**: Recorded fixtures yield identical decoded results across repeated runs and can validate
  at least 95% of viewer behavior without physical hardware.

## Assumptions

- Version 1 is Android-first and targets the existing Galaxy A54/ST-Link V3 workflow; the decoding
  model and recorded fixtures remain portable for a later desktop or Rust consumer.
- The demonstration firmware will expose a deliberately designed, coherent debug snapshot in
  readable SRAM; arbitrary application variables will not be discovered heuristically.
- The supplied `.mmap` is a navigation catalog with known missing source provenance, not a normative
  replacement for the STM32 reference manual or a versioned SVD.
- Direct register inspection is secondary to target-owned snapshots and is disabled until each
  register group receives an explicit read-safety review.
- Version 1 does not modify peripheral settings. Editing, tuning, scripting, programming, and target
  control are out of scope.
- Existing simulator and board-viewer work is not a dependency for
  the Android-first vertical slice.

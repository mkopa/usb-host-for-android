# Research: MK3 Realtime Viewer Transport

## Decision 1: Snapshot-first observation

**Decision**: Read coherent, versioned MK3 snapshots as the primary data source. Permit direct
register reads only from an explicit safe-read catalog.

**Rationale**: A target-owned snapshot avoids torn multi-register state and read-side effects while
remaining traceable to raw evidence.

**Alternatives considered**: Broad live register polling was rejected because access type alone does
not prove that a peripheral register is safe to read repeatedly.

## Decision 2: Reuse the stable C ABI

**Decision**: Add observation capabilities below Java/JNI to the existing fixed-width C ABI rather
than publishing a C++ ABI or an Android-only interface.

**Rationale**: Android, native C++, desktop experiments, and future Rust bindings can share the same
ownership, status, and compatibility rules.

**Alternatives considered**: JNI-only access was rejected because it couples decoding to Android;
a public C++ ABI was rejected because compiler/STL compatibility is weaker.

## Decision 3: Android support claims are evidence-scoped

**Decision**: Mark Android USB OTG + STLINK-V3 as supported using the dated Galaxy A54 evidence.
Describe Windows, Linux, macOS, and Rust consumption as experimental/in validation until each has a
backend and its own recorded contract/hardware results.

**Rationale**: Portable source is not the same as a validated platform transport.

**Alternatives considered**: A broad cross-platform support claim was rejected as unverifiable.

## Decision 4: Adaptive, bounded sampling

**Decision**: Default to a 5 Hz coherent dashboard, reduce polling under lifecycle/bandwidth
pressure, and attach freshness and generation metadata to every value.

**Rationale**: Human-readable state matters more than maximizing raw SWD traffic, and stale values
must never look live.

**Alternatives considered**: Fixed maximum-rate polling was rejected because it obscures overload
and wastes mobile power.

## Decision 5: Preserve the hard read-only boundary

**Decision**: The observation API exposes no flash program, erase, option-byte write, memory write,
reset, halt, run, step, or arbitrary register-write operation.

**Rationale**: Safety must be guaranteed below the UI. A future controlled-write feature requires a
separate specification, explicit confirmation, and recovery design.

**Alternatives considered**: Hidden or role-gated write calls in the same ABI were rejected because
UI authorization is not a transport safety boundary.

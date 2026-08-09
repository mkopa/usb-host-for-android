# Data Model: STM32G0 Realtime Viewer

## Observation Session

| Field | Type | Rules |
|---|---|---|
| session | opaque 64-bit handle | Non-zero and process-local |
| state | enum | `OPENING`, `READY`, `OBSERVING`, `STALE`, `FAILED`, `CLOSED` |
| targetIdentity | Target Identity | Must remain stable for the session |
| requestedRateHz | unsigned integer | Bounded policy input |
| effectiveRateHz | measured value | Reported, never inferred by UI |
| lastGeneration | unsigned 64-bit | Strictly increasing accepted sample |

Detachment, permission loss, identity change, or an unrecoverable transport error terminates live
observation. Close remains idempotent.

## Project Snapshot

| Field | Type | Rules |
|---|---|---|
| magic | fixed bytes | Identifies the demonstration snapshot family |
| schemaVersion | unsigned integer | Unsupported versions are rejected |
| generation | unsigned 64-bit | Same before and after payload read |
| payloadLength | unsigned integer | Bounded by contract |
| payloadCrc32 | unsigned 32-bit | Must match the complete payload |
| firmwareIdentity | bounded metadata | Detects target/firmware replacement |
| timestamp | monotonic timestamp | Source for freshness |

## Observed Value

| Field | Type | Rules |
|---|---|---|
| key | stable string/ID | Unique within decoder version |
| category | enum | Configuration, state, counter, warning, unknown |
| value | typed scalar/enum | Human-readable representation |
| unit | optional string | Stable engineering unit |
| evidence | source descriptor | Symbol/address, raw value, bit range |
| quality | enum | Current, stale, unavailable, invalid, undecoded |
| generation | unsigned 64-bit | Links to one coherent snapshot |

## Register Catalog Entry

Each entry records peripheral, register, address, width, access metadata, fields, provenance, and
`SAFE_SNAPSHOT`, `SAFE_DIRECT`, `READ_SIDE_EFFECT`, `WRITE_ONLY`, or `UNCLASSIFIED`. Only
`SAFE_DIRECT` can produce a direct transport request.

## Sample History

A bounded ordered ring of accepted snapshots and change summaries. Freeze selects a generation but
does not stop collection; resume returns to the newest coherent generation. Raw target blocks are
excluded from export unless explicitly opted in.

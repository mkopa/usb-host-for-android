# Data Model: Generic USB Transport

## Transport Session

Represents one native session created from an Android-authorized connection.

**Fields**:

- opaque 64-bit handle with registry generation
- duplicated native file descriptor
- immutable device descriptor snapshot
- available configuration snapshots
- active configuration value and descriptor generation
- claimed interface numbers and active alternate settings
- optional active transfer operation
- lifecycle state and last sanitized terminal error

**Lifecycle**:

```text
OPENING → OPEN → CLOSING → CLOSED
             └→ FAILED → CLOSING → CLOSED
```

- `OPENING → OPEN` only after FD duplication, libusb wrap, and descriptor snapshot succeed.
- Any failed open closes partial native resources and publishes no handle.
- `OPEN → CLOSING` rejects new operations and cancels an active transfer.
- `CLOSING → CLOSED` occurs within 2 seconds after completion is observed and resources are released.
- `close(CLOSED)` is an idempotent no-op; stale handle generations return invalid state.

## Device Descriptor Snapshot

**Fields**: USB version, device class/subclass/protocol, endpoint-zero packet size, vendor/product
IDs, device release, and configuration snapshots. String values and serial number are not fetched or
logged by default.

**Identity**: Belongs to exactly one transport-session snapshot generation. It is not a persistent
device identity.

## Configuration Snapshot

**Fields**: configuration value, attributes, maximum power, interface groups, additional descriptors,
active flag, and snapshot generation.

**Rules**:

- Configuration values are unique within a device snapshot.
- Selection is explicit and valid only with zero claimed interfaces.
- Successful selection increments the generation and refreshes active settings and endpoints.
- Failure leaves the previous active model unchanged.

## Interface Group

Groups all alternate settings for one interface number.

**Fields**: interface number, ordered alternate-setting snapshots, claimed flag, and selected
alternate-setting number when active.

**Rules**:

- Interface numbers are unique inside a configuration snapshot.
- Claim is exclusive per session and idempotent only for the same live managed wrapper.
- Release invalidates the corresponding `GenericUsbInterface` wrapper.

## Alternate-Setting Snapshot

**Fields**: interface number, alternate-setting number, class/subclass/protocol, endpoint snapshots,
additional descriptors, and snapshot generation.

**Rules**:

- Alternate-setting numbers are unique within an interface group.
- Selection requires the interface to be claimed.
- Successful selection invalidates endpoints from the previous generation.

## Endpoint Snapshot

**Fields**: address, endpoint number, direction, transfer type, maximum packet size, polling interval,
additional descriptors, parent interface/alternate setting, and snapshot generation.

**Rules**:

- Address and transfer type must match the active descriptor at dispatch time.
- Endpoint zero is reserved for control transfers and is not a claimable endpoint object.
- Isochronous endpoints can be inspected but cannot execute v1 transfers.
- A stale generation, wrong parent session, or unclaimed parent interface is invalid state.

## Additional Descriptor

**Fields**: descriptor type byte and immutable raw byte payload including the original descriptor
header.

**Rules**:

- Attached only at the configuration, alternate-setting, or endpoint scope reported by libusb.
- Individual and aggregate copies are bounded by the 16-bit USB configuration total length.
- Malformed lengths or aggregate overflow fail snapshot creation without exposing partial data.

## Claimed Interface

Managed lifecycle token referencing a session handle, interface number, and claim generation.

**Lifecycle**:

```text
CREATED → CLAIMED → RELEASED
                    ↑
       session close/disconnect
```

Close/release is idempotent. Calls after release or parent-session closure return invalid state.

## Transfer Request

Common fields: session, transfer type, direction, caller buffer, offset, requested length, timeout,
and expected descriptor generation.

- **Control** adds request type, request, value, and index; length is 0–65,535 bytes.
- **Bulk/interrupt** adds endpoint address; length is 0–1 MiB.
- Timeout is always 1–60,000 ms.
- Offset plus length must fit the caller buffer without overflow.

## Transfer Operation

Internal cancellable execution object.

**Fields**: request snapshot, libusb/fake backend token, completion condition, operation state,
actual count, terminal status, and sanitized diagnostic.

**Lifecycle**:

```text
CREATED → SUBMITTED → COMPLETED
                 ├→ CANCELLING → CANCELLED
                 ├→ TIMED_OUT
                 ├→ STALLED
                 └→ DISCONNECTED
```

Only one operation may be submitted per session. Cancellation can report a non-zero partial count;
the caller must not assume zero bytes moved.

## Transfer Result

**Fields**: stable status, actual byte count, and bounded sanitized diagnostic context.

**Rules**:

- Actual count never exceeds requested length.
- Short successful packets remain success.
- IN transfers modify only the actual-count portion of the requested slice.
- Diagnostics exclude serial number, USB filesystem path, raw descriptor dump, and stable device ID.

## Protocol Adapter

Owns protocol/class logic and references a public generic session/interface. It does not own Android
permission, implement JNI, access raw libusb types, or bypass transport validation.

## GitHub Task Work Item

Delivery metadata for one generated task.

**Fields**: task ID, issue number/URL, dependencies, branch `feat/<issue>-<task>-<slug>`, PR URL,
local-verification state/evidence, issue state, and merge commit.

**State transitions**:

```text
TASK GENERATED → ISSUE OPEN → IN PROGRESS → PR OPEN → LOCAL CHECKS PASS → MERGED/CLOSED
                                              └→ LOCAL CHECKS FAIL → UPDATED/RETRY
```

No implementation branch starts before dependency issues are merged. A new split task creates a new
issue before its branch is created.

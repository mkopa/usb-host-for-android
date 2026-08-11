# Research: Generic USB Transport

## Decision 1: Reuse the existing AAR and Prefab module

**Decision**: Add `info.marcin.usbhost.transport` and `usbhost/transport.h` to the existing
`usbHostForAndroid` artifact and `usbhost` Prefab package.

**Rationale**: One artifact avoids duplicate libusb runtimes, native symbol collisions, and a second
consumer dependency. It preserves the current Maven coordinate and lets STLINK share the transport.

**Alternatives considered**: A second Gradle module/artifact was rejected because it would duplicate
native packaging or require a more complex shared-native dependency with no current product benefit.

## Decision 2: Synchronous public calls over asynchronous libusb operations

**Decision**: Keep managed/C calls synchronous, but implement Android transfers with libusb async
transfer objects, cancellation, callbacks, and a shared event-handling thread.

**Rationale**: Local libusb 1.0.30 documents that cancellation is asynchronous and completes through
the callback; a synchronous libusb call cannot independently satisfy the accepted close-during-I/O
contract. A shared event loop supports cancellation and concurrent devices without one thread per
session.

**Alternatives considered**: Direct synchronous libusb functions were rejected because close would
wait for the operation timeout. Per-session event threads were rejected due to resource cost and
more difficult shutdown ordering. A public callback/coroutine API was deferred because applications
already control scheduling and the accepted v1 contract is synchronous.

## Decision 3: Duplicate the authorized Android descriptor

**Decision**: Native open duplicates `UsbDeviceConnection.getFileDescriptor()`, wraps only that
duplicate, and closes it after `libusb_close()`.

**Rationale**: Local libusb documentation requires the wrapped FD to stay open and states that
`libusb_close()` does not close it. Duplication isolates native lifetime while leaving the caller's
Android connection ownership unchanged.

**Alternatives considered**: Borrowing the caller FD was rejected because lifecycle closure could
invalidate in-flight native work. Transferring ownership was rejected as a breaking and surprising
Android API contract.

## Decision 4: One process-wide reference-counted libusb runtime

**Decision**: Initialize one context with `LIBUSB_OPTION_NO_DEVICE_DISCOVERY`, run one event thread,
and retain it while at least one generic/STLINK-backed session exists.

**Rationale**: It keeps Android permission authoritative, avoids device scanning, amortizes the
event thread, and lets sessions operate concurrently while serializing only their own calls.

**Alternatives considered**: A context per session works but multiplies threads and complicates
global shutdown. The default libusb context was rejected because explicit ownership and options are
harder to verify.

## Decision 5: Snapshot descriptors into owned portable values

**Decision**: Copy standard descriptor fields and bounded additional descriptors into C++ values.
Expose additional data as immutable `type + raw bytes` records scoped to configuration,
alternate-setting, or endpoint.

**Rationale**: libusb descriptor allocations and pointers cannot cross ABI/JNI boundaries. The raw
record preserves inputs needed by future HID/DFU/class adapters without exporting libusb structures.

**Alternatives considered**: A whole raw configuration blob was rejected because every consumer
would need unsafe duplicate parsing. Standard fields only were rejected because future adapters
would need private JNI/native escape hatches.

## Decision 6: Explicit configuration and generation-bound endpoints

**Decision**: Permit explicit configuration selection only with zero claimed interfaces. Refresh
active settings/endpoints after selection and reject endpoint objects from an older generation.

**Rationale**: No hidden device-state mutation occurs, and stale endpoint handles cannot address a
different configuration or alternate setting accidentally.

**Alternatives considered**: Automatic configuration selection was rejected as surprising and
potentially destructive. Active-configuration-only support was rejected as insufficiently generic.

## Decision 7: Stable additive ABI from first publication

**Decision**: Preserve status numbers and meanings, append new statuses/functions, prefix public
structs with `struct_size`, use fixed-width types and caller-owned buffers, and require a new major
version for incompatible changes.

**Rationale**: C++ adapters and future Rust bindings need an allocator-neutral, exception-free ABI.
Stability is an accepted requirement even before the enclosing library reaches 1.0.

**Alternatives considered**: Experimental C ABI was rejected because it would undermine the stated
foundation. Exposing C++ classes was rejected because compiler/STL ABI and exceptions are unstable.

## Decision 8: No new runtime dependency

**Decision**: Use Java/JNI/C++ standard facilities and the pinned libusb/STLINK sources already in
the repository.

**Rationale**: Current dependencies cover USB I/O, event handling, synchronization, and testing.
An added runtime or coroutine dependency is unnecessary for the synchronous public API.

**Alternatives considered**: RxJava/coroutine runtime, a second USB stack, or an external descriptor
parser would expand the artifact and maintenance/licensing surface without satisfying a missing need.

## Decision 9: Fake-first automated validation

**Decision**: Make the portable core depend on a backend interface and test it with a scripted fake
for descriptors, partial transfers, timeout, stall, cancellation, detach, races, and limits.

**Rationale**: Deterministic tests can cover failure ordering and concurrency without modifying or
depending on physical devices. Hardware checks only validate explicitly named adapters and remain
non-destructive.

**Alternatives considered**: Hardware-only integration tests were rejected as nondeterministic and
unsafe. Mocking JNI alone was rejected because it would not exercise the native state machine.

## Decision 10: Issue/branch/PR lifecycle is task-granular

**Decision**: After task regeneration, sync every task to one GitHub issue. Implement each ready task
on `feat/<issue>-<task>-<slug>`, open one PR to `dev`, update its issue, and merge after recorded local
checks pass. GitHub Actions remain disabled until the maintainer explicitly restores them.

**Rationale**: This implements the requested traceability and makes every change independently
reviewable. Dependency metadata prevents branches from starting too early.

**Alternatives considered**: One feature PR was rejected by explicit workflow requirement. Hosted
Actions were rejected temporarily because execution is unavailable; retaining workflow files while
using recorded local verification preserves future reactivation without blocking delivery.

## Operational prerequisite

The repository has issues enabled and the authenticated actor has `ADMIN`. Hosted Actions are
disabled at repository level; native, Android, publication, and policy verification runs locally and
is recorded in each issue and PR before merge.

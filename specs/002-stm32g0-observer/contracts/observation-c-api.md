# Contract: Observation C API

The observation API extends `usbhost/usbhost.h` without changing existing ABI v1 values.

## Compatibility

- New functions and size-prefixed structures require a new advertised ABI capability/version.
- Existing enum values are never renumbered.
- No exceptions, STL types, JNI objects, Android types, or borrowed diagnostic buffers cross calls.
- The caller owns output buffers; the session owns transport state and serializes operations.

## Operations

1. Open an explicitly authorized STLINK-V3 file descriptor.
2. Connect and validate target identity.
3. Start/stop a bounded observation policy.
4. Read one complete snapshot into a caller-owned buffer.
5. Query freshness, generation, effective rate, and stable status codes.
6. Close idempotently and cancel/wait for in-flight observation work.

Snapshot reads reject invalid magic, unsupported schema, changing generation, out-of-range length,
CRC failure, target replacement, disconnect, and cancellation. The contract does not expose target
write, erase, reset, halt, run, or step operations.

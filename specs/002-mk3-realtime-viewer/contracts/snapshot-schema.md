# Contract: MK3 Snapshot Envelope

Each snapshot is a little-endian, bounded envelope owned by running MK3 firmware.

| Field | Required validation |
|---|---|
| Magic | Exact MK3 snapshot identifier |
| Schema version | Explicitly supported version |
| Header size | At least known header, no payload overlap |
| Generation begin/end | Equal and newer than last accepted generation |
| Payload length | Within mapped snapshot region and caller capacity |
| Payload CRC32 | Matches the complete payload |
| Firmware identity | Matches the active observation session |

Readers first capture the header, then the bounded payload, then re-read the generation/commit
marker. A mismatch is a torn snapshot and is retried within the sampling budget, never displayed.
Unknown fields in a compatible newer envelope may be skipped only when its declared sizes preserve
all known fields. Unknown schema semantics produce `UNSUPPORTED`, never guessed values.

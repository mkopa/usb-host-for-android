# Data Model: ST-Link Android Host

## Programmer Descriptor

| Field | Type | Rules |
|-------|------|-------|
| deviceId | integer | Android connection-local identifier |
| deviceName | string | Stable only for the current attachment |
| vendorId | unsigned 16-bit | Must equal `0x0483` |
| productId | unsigned 16-bit | Must be a supported ST-Link V3 debug PID |
| serial | optional string | Returned only after permission; not logged by default |
| support | enum | `SUPPORTED`, `UNSUPPORTED_MODE`, or `UNSUPPORTED_DEVICE` |

Identity for selection is `(deviceId, deviceName, vendorId, productId)`. A serial number is display
metadata and never the sole attachment key.

## Programmer Session

| Field | Type | Rules |
|-------|------|-------|
| handle | opaque unsigned 64-bit | Non-zero, process-local, never reused while live |
| state | enum | See state machine below |
| programmerVersion | integer tuple | ST-Link major and firmware components |
| descriptorFd | owned integer | Duplicated at open; closed once after libusb closes |
| target | optional Target Descriptor | Present only after successful target connection |
| lastError | Operation Error | Replaced by each failing operation on the current thread |

### Session state machine

```text
OPENING -> PROGRAMMER_READY -> TARGET_READY
   |              |                 |
   +--------------+-----------------+-> FAILED -> CLOSED
                  +------------------------------> CLOSED
```

- `OPENING` is not externally visible.
- `connectTarget` is valid only in `PROGRAMMER_READY`; repeated success may return cached data.
- `readMemory` is valid only in `TARGET_READY`.
- Transport loss moves the session to `FAILED`; only `close` is then accepted.
- `close` from any externally visible state is idempotent and ends in `CLOSED`.

## Target Descriptor

| Field | Type | STM32G0B0RET6 rule |
|-------|------|--------------------|
| chipId | unsigned 32-bit | Must equal `0x467` |
| family | enum/string | `STM32G0Bx_G0Cx` |
| flashBase | unsigned 32-bit | `0x08000000` |
| flashSize | unsigned 32-bit | Read from device; expected 512 KiB for `RET6` |
| flashPageSize | unsigned 32-bit | 2 KiB |
| sramBase | unsigned 32-bit | `0x20000000` |
| sramSize | unsigned 32-bit | 144 KiB |
| targetVoltageMv | signed 32-bit | Positive millivolts; `-1` if unavailable |

## Memory Range

| Field | Type | Rules |
|-------|------|-------|
| address | unsigned 32-bit | Start in target flash or SRAM |
| length | unsigned 32-bit | `1..1,048,576` bytes |
| endExclusive | unsigned 64-bit derived | Must not overflow or exceed the selected region |

Reads may be unaligned. The native adapter aligns internal transfers and returns only requested bytes.

## Operation Error

| Field | Type | Rules |
|-------|------|-------|
| status | stable integer enum | Never renumber existing values |
| message | optional UTF-8 | Diagnostic, valid until next call on the same thread |
| terminal | boolean derived | USB disconnect and unrecoverable backend errors terminalize session |

## Hardware Evidence Record

| Field | Type | Rules |
|-------|------|-------|
| date | ISO date | Required |
| Android device/version | string | Required |
| library revision | commit | Required |
| ST-Link model/firmware | string | Required |
| target/board | string | Required |
| connection conditions | string | Power, wiring, SWD speed |
| scenarios | checklist | Identify and required read sizes |
| result | pass/fail | Failures include logs with identifiers redacted |

# ST-Link V3 / STM32G0B0RET6 Hardware Evidence

**Date**: YYYY-MM-DD
**Result**: PASS / FAIL
**Library revision**: commit

## Environment

- Android device model:
- Android version / API:
- CPU ABI:
- ST-Link V3 model:
- ST-Link firmware (V/J/S):
- Target board / MCU marking:
- Target power and measured VTref:
- SWD wiring and cable length:
- SWD frequency:

Do not record USB serial numbers or raw target memory contents in a shared evidence file.

## Scenarios

- [ ] Android discovers exactly the expected supported programmer.
- [ ] Permission denial returns `PERMISSION_DENIED` and leaves no session.
- [ ] Programmer opens without a connected target.
- [ ] Target identification reports chip ID `0x467` twenty consecutive times.
- [ ] Flash reports 512 KiB, SRAM 144 KiB, and page size 2 KiB.
- [ ] Reads of 1 B, 4 B, 1 KiB, and 64 KiB match an independent SHA-256 twenty times each.
- [ ] Invalid and overflowing ranges perform no USB read.
- [ ] Detach during read returns `DISCONNECTED`; repeated close is safe.

## Observations

Record timings, redacted diagnostics, unexpected behavior, and recovery steps here.

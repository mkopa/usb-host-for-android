# RtlSdrForAndroid example

This application demonstrates Android USB permission handling and the first non-mutating part of an
RTL-SDR connection through `info.marcin.usbhost.transport`:

1. detect a common RTL2832U USB ID;
2. request permission through `UsbManager`;
3. open the authorized file descriptor through `GenericUsbDevice`;
4. claim interface 0 and display its descriptor and endpoints;
5. release the interface and close both session owners on disconnect.

The connection probe itself performs no device mutation. Its optional FM player then uses the
official rtl-sdr R82xx tuner driver and a native port of the `rtl_fm -M wbfm` pipeline to configure
volatile registers, tune 93.9 MHz, stream IQ over endpoint `0x81`, produce 32 kHz mono PCM, and
feed Android `AudioTrack`. Kotlin does not process I/Q samples.
It does not write EEPROM. The initial USB IDs are `0BDA:2832` and `0BDA:2838`.

The Android library remains MIT-licensed. Because the FM example compiles the GPL-2.0-or-later
R82xx driver from the pinned official rtl-sdr submodule, the resulting example application is
distributed under GPL-2.0-or-later; see `src/main/cpp/third_party/rtl-sdr/COPYING`.

Build the APK locally with:

```powershell
./gradlew.bat :rtlSdrForAndroid:assembleDebug
```

## Hardware validation

Validated on 2026-08-12 with a Samsung Galaxy A54 running Android 16 and a generic
`0BDA:2838` RTL2832U dongle with an R820T/R820T2 tuner. The 93.9 MHz WBFM stream produced clear,
continuous audio through Android `AudioTrack`; no EEPROM write was performed.

The dated record, its NOT RUN rows, and its scope deviation from the generic smoke procedure are in
[`docs/hardware/2026-08-12-samsung-a54-rtlsdr-r820t.md`](../docs/hardware/2026-08-12-samsung-a54-rtlsdr-r820t.md).
This record covers the example only; it does not establish generic RTL-SDR class support.

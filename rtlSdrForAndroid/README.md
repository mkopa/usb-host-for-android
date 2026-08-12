# RtlSdrForAndroid example

This application demonstrates Android USB permission handling and the first non-mutating part of an
RTL-SDR connection through `info.marcin.usbhost.transport`:

1. detect a common RTL2832U USB ID;
2. request permission through `UsbManager`;
3. open the authorized file descriptor through `GenericUsbDevice`;
4. claim interface 0 and display its descriptor and endpoints;
5. release the interface and close both session owners on disconnect.

The connection probe itself performs no device mutation. Its optional FM player then uses the
official rtl-sdr R82xx tuner driver and the `rtl_fm` WBFM profile to configure volatile registers,
tune 93.9 MHz, stream IQ over endpoint `0x81`, demodulate mono FM, and feed Android `AudioTrack`.
It does not write EEPROM. The initial USB IDs are `0BDA:2832` and `0BDA:2838`.

The Android library remains MIT-licensed. Because the FM example compiles the GPL-2.0-or-later
R82xx driver from the pinned official rtl-sdr submodule, the resulting example application is
distributed under GPL-2.0-or-later; see `src/main/cpp/third_party/rtl-sdr/COPYING`.

Build the APK locally with:

```powershell
./gradlew.bat :rtlSdrForAndroid:assembleDebug
```

# RtlSdrForAndroid example

This application demonstrates Android USB permission handling and the first non-mutating part of an
RTL-SDR connection through `info.marcin.usbhost.transport`:

1. detect a common RTL2832U USB ID;
2. request permission through `UsbManager`;
3. open the authorized file descriptor through `GenericUsbDevice`;
4. claim interface 0 and display its descriptor and endpoints;
5. release the interface and close both session owners on disconnect.

The example intentionally performs no vendor control transfer, tuner/baseband initialization,
streaming, USB reset, EEPROM access, or device write. Its initial USB IDs correspond to the common
generic RTL2832U entries recognized by rtl-sdr: `0BDA:2832` and `0BDA:2838`.

Build the APK locally with:

```powershell
./gradlew.bat :rtlSdrForAndroid:assembleDebug
```

# Java API Contract

Package: `dev.usbhost.android`

## `StlinkProber`

```java
public static boolean isSupported(UsbDevice device);
public static List<StlinkDevice> findAll(UsbManager manager);
```

`findAll` is deterministic by Android device ID and returns immutable descriptor objects. It does not
request permission or open devices.

## `StlinkDevice`

```java
public UsbDevice getUsbDevice();
public int getVendorId();
public int getProductId();
public String getDeviceName();
public StlinkSession open(UsbManager manager, int swdFrequencyKhz) throws StlinkException;
```

Open requires prior permission and a worker thread. The returned session owns the opened
`UsbDeviceConnection`; on failure the connection is closed before the exception returns.

## `StlinkSession`

```java
public synchronized ProgrammerInfo getProgrammerInfo();
public synchronized TargetInfo connectTarget() throws StlinkException;
public synchronized byte[] readMemory(long address, int length) throws StlinkException;
public synchronized boolean isOpen();
@Override public synchronized void close();
```

- `connectTarget` uses non-resetting hot-plug connection.
- `readMemory` is available only after successful target connection.
- Blocking methods reject the Android main thread with `INVALID_STATE`.
- `close` is idempotent and safe after transport errors.

## Value and error types

- `ProgrammerInfo`: ST-Link major version, JTAG firmware version, and SWIM firmware version.
- `TargetInfo`: chip ID, family, flash/SRAM base and size, flash page size, and target millivolts.
- `StlinkException`: stable `StlinkStatus`, redacted diagnostic message, and no raw memory contents.
- `StlinkStatus`: Java enum mirroring the C numeric status values exactly.

# Managed API Contract

## Package

All types below are public under `info.marcin.usbhost.transport` and are consumable from Java and
Kotlin. Public value types are immutable final classes rather than Kotlin-only or Android-internal
types.

## GenericUsbDevice

```java
public final class GenericUsbDevice implements AutoCloseable {
    public static GenericUsbDevice open(
            UsbDevice device,
            UsbDeviceConnection connection) throws UsbTransportException;

    public GenericUsbDeviceDescriptor getDescriptor();
    public List<GenericUsbConfiguration> getConfigurations();
    public GenericUsbConfiguration getActiveConfiguration();

    public void selectConfiguration(int configurationValue) throws UsbTransportException;
    public GenericUsbInterface claimInterface(int interfaceNumber)
            throws UsbTransportException;

    public UsbTransferResult controlTransfer(
            UsbControlRequest request,
            byte[] buffer,
            int offset,
            int length,
            int timeoutMillis) throws UsbTransportException;

    public void cancelActiveTransfer() throws UsbTransportException;
    public boolean isOpen();

    @Override
    public void close() throws UsbTransportException;
}
```

`open` verifies non-null matching device/connection inputs, runs off the main thread, and never closes
the caller's connection. Returned lists are unmodifiable snapshots. `close` is safe to repeat.

## GenericUsbInterface

```java
public final class GenericUsbInterface implements AutoCloseable {
    public int getInterfaceNumber();
    public GenericUsbAlternateSetting getActiveAlternateSetting();
    public void selectAlternateSetting(int alternateSetting) throws UsbTransportException;

    public UsbTransferResult bulkTransfer(
            GenericUsbEndpoint endpoint,
            byte[] buffer,
            int offset,
            int length,
            int timeoutMillis) throws UsbTransportException;

    public UsbTransferResult interruptTransfer(
            GenericUsbEndpoint endpoint,
            byte[] buffer,
            int offset,
            int length,
            int timeoutMillis) throws UsbTransportException;

    public boolean isClaimed();

    @Override
    public void close() throws UsbTransportException;
}
```

The wrapper is bound to its parent session and claim generation. Stale or foreign endpoints fail
before native submission. Release/close is idempotent.

## Immutable descriptor types

- `GenericUsbDeviceDescriptor`: USB/device versions, class/subclass/protocol, endpoint-zero packet
  size, vendor/product IDs, and configuration count.
- `GenericUsbConfiguration`: value, attributes, maximum power, active flag, interface descriptors,
  and additional descriptors.
- `GenericUsbInterfaceDescriptor`: interface number and available alternate settings.
- `GenericUsbAlternateSetting`: number, class/subclass/protocol, endpoints, and additional
  descriptors.
- `GenericUsbEndpoint`: address, endpoint number, direction, transfer type, maximum packet size,
  interval, additional descriptors, and internal snapshot generation.
- `AdditionalUsbDescriptor`: unsigned descriptor type and defensive copy of bounded raw bytes.

The public API exposes getters and value equality/hash behavior. Arrays are returned as defensive
copies and collections are unmodifiable.

## Transfer types

- `UsbDirection`: `IN`, `OUT`.
- `UsbTransferType`: `CONTROL`, `BULK`, `INTERRUPT`, `ISOCHRONOUS` (metadata only for the last value).
- `UsbControlRequest`: immutable request type, request, value, index, and direction with unsigned
  USB-field validation.
- `UsbTransferResult`: stable status and actual byte count; successful short packets remain `OK`.
- `UsbTransportStatus`: numeric mapping identical to the C ABI.
- `UsbTransportException`: stable status, actual/partial byte count, and sanitized message; it does
  not expose raw libusb values as the primary contract.

## Main-thread and concurrency behavior

`open`, configuration/claim/alternate changes, transfers, cancellation, interface release, and
device close reject Android's main Looper before entering JNI. Descriptor getters and state queries
are non-blocking. A transfer does not hold a Java monitor that prevents a second worker thread from
calling cancellation or close.

## Existing API compatibility

No existing public `info.marcin.usbhost` class, constructor visibility, method signature, status
number, connection-ownership rule, or read-only safety restriction changes. STLINK reuse of the
transport is internal.

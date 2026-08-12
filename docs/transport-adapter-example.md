# Minimal Transport Adapter

`info.marcin.usbhost.transport` supplies permission-safe USB lifecycle and transfer primitives. It
does not implement a device protocol. An adapter must select interfaces and endpoints from a
documented, independently validated protocol profile; successfully opening a generic device is not
a support claim.

Android `UsbManager` remains responsible for discovery, permission, and opening the caller-owned
`UsbDeviceConnection`. Every blocking call below must run on a worker thread.

## Java adapter

This minimal adapter accepts interface and endpoint addresses from a verified profile. It neither
scans native USB devices nor contains JNI code.

```java
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import info.marcin.usbhost.transport.*;

final class BulkProtocolAdapter implements AutoCloseable {
    private final GenericUsbDevice device;
    private final GenericUsbInterface claimed;
    private final GenericUsbEndpoint endpointOut;
    private final GenericUsbEndpoint endpointIn;

    static BulkProtocolAdapter open(UsbDevice usb, UsbDeviceConnection connection,
            int interfaceNumber, int endpointOut, int endpointIn) throws UsbTransportException {
        GenericUsbDevice device = GenericUsbDevice.open(usb, connection);
        try {
            GenericUsbInterface claimed = device.claimInterface(interfaceNumber);
            GenericUsbEndpoint out = find(claimed, endpointOut, UsbDirection.OUT);
            GenericUsbEndpoint in = find(claimed, endpointIn, UsbDirection.IN);
            return new BulkProtocolAdapter(device, claimed, out, in);
        } catch (Exception error) {
            device.close();
            throw error;
        }
    }

    private static GenericUsbEndpoint find(GenericUsbInterface claimed, int address,
            UsbDirection direction) throws UsbTransportException {
        for (GenericUsbEndpoint endpoint : claimed.getActiveAlternateSetting().getEndpoints()) {
            if (endpoint.getAddress() == address && endpoint.getDirection() == direction
                    && endpoint.getTransferType() == UsbTransferType.BULK) return endpoint;
        }
        throw new UsbTransportException(
                UsbTransportStatus.INVALID_ARGUMENT, "Required bulk endpoint is unavailable");
    }

    private BulkProtocolAdapter(GenericUsbDevice device, GenericUsbInterface claimed,
            GenericUsbEndpoint endpointOut, GenericUsbEndpoint endpointIn) {
        this.device = device;
        this.claimed = claimed;
        this.endpointOut = endpointOut;
        this.endpointIn = endpointIn;
    }

    byte[] exchange(byte[] request, int replyCapacity) throws UsbTransportException {
        claimed.bulkTransfer(endpointOut, request, 0, request.length, 3_000);
        byte[] reply = new byte[replyCapacity];
        UsbTransferResult result = claimed.bulkTransfer(
                endpointIn, reply, 0, reply.length, 3_000);
        return java.util.Arrays.copyOf(reply, result.getActualLength());
    }

    @Override public void close() throws UsbTransportException {
        try { claimed.close(); } finally { device.close(); }
    }
}
```

`UsbTransferResult.getActualLength()` must be honored for short transfers. A thrown
`UsbTransportException` also carries an actual length when the backend completed partially.

## Kotlin use

```kotlin
withContext(Dispatchers.IO) {
    BulkProtocolAdapter.open(device, connection, 0, 0x01, 0x81).use { adapter ->
        val reply = adapter.exchange(byteArrayOf(0x01, 0x02), 64)
        // Decode only according to the adapter's validated protocol.
    }
}
// GenericUsbDevice did not close the caller-owned UsbDeviceConnection.
connection.close()
```

Use `selectConfiguration` only before claiming any interface. After changing configuration or an
alternate setting, discard previously captured endpoint objects because their snapshot generation
is stale. Call `cancelActiveTransfer` from a coordinating worker when shutdown must interrupt a
blocking operation; `close` also performs bounded cancellation and cleanup.

## Capability boundary

The transport provides descriptor inspection, explicit configuration/interface state, control,
bulk, and interrupt transfers. It does not provide isochronous transfers, native discovery, Android
permission bypass, or automatic compatibility with serial, DFU, CMSIS-DAP, HID, printers,
analyzers, or other programmers. Each adapter needs its own tests and recorded hardware validation.

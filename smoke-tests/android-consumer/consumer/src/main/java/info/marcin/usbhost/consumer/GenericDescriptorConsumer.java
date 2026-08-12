package info.marcin.usbhost.consumer;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;

import java.util.List;

import info.marcin.usbhost.transport.GenericUsbConfiguration;
import info.marcin.usbhost.transport.GenericUsbDevice;
import info.marcin.usbhost.transport.GenericUsbDeviceDescriptor;
import info.marcin.usbhost.transport.UsbTransportException;

/** Compile-only consumer of the published generic descriptor and caller-ownership contract. */
public final class GenericDescriptorConsumer {
    private GenericDescriptorConsumer() {}

    public static Snapshot inspectCallerOwnedConnection(
            UsbDevice device, UsbDeviceConnection callerOwnedConnection)
            throws UsbTransportException {
        try (GenericUsbDevice session = GenericUsbDevice.open(device, callerOwnedConnection)) {
            GenericUsbDeviceDescriptor descriptor = session.getDescriptor();
            return new Snapshot(descriptor.getVendorId(), descriptor.getProductId(),
                    session.getConfigurations());
        }
    }

    public static final class Snapshot {
        public final int vendorId;
        public final int productId;
        public final List<GenericUsbConfiguration> configurations;

        Snapshot(int vendorId, int productId, List<GenericUsbConfiguration> configurations) {
            this.vendorId = vendorId;
            this.productId = productId;
            this.configurations = configurations;
        }
    }
}

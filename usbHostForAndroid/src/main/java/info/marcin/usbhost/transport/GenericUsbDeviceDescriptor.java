package info.marcin.usbhost.transport;

import java.util.List;
import java.util.Objects;

/** Immutable device descriptor and configuration snapshot with no native pointer lifetime. */
public final class GenericUsbDeviceDescriptor {
    private final int usbVersionBcd;
    private final int deviceClass;
    private final int deviceSubclass;
    private final int deviceProtocol;
    private final int endpointZeroMaximumPacketSize;
    private final int vendorId;
    private final int productId;
    private final int deviceReleaseBcd;
    private final List<GenericUsbConfiguration> configurations;
    private final long snapshotGeneration;

    GenericUsbDeviceDescriptor(int usbVersionBcd, int deviceClass, int deviceSubclass,
            int deviceProtocol, int endpointZeroMaximumPacketSize, int vendorId, int productId,
            int deviceReleaseBcd, List<GenericUsbConfiguration> configurations,
            long snapshotGeneration) {
        this.usbVersionBcd = ManagedDescriptorSupport.unsigned(usbVersionBcd, 0xffff, "usbVersionBcd");
        this.deviceClass = ManagedDescriptorSupport.unsigned(deviceClass, 0xff, "deviceClass");
        this.deviceSubclass = ManagedDescriptorSupport.unsigned(deviceSubclass, 0xff, "deviceSubclass");
        this.deviceProtocol = ManagedDescriptorSupport.unsigned(deviceProtocol, 0xff, "deviceProtocol");
        this.endpointZeroMaximumPacketSize = ManagedDescriptorSupport.unsigned(
                endpointZeroMaximumPacketSize, 0xff, "endpointZeroMaximumPacketSize");
        this.vendorId = ManagedDescriptorSupport.unsigned(vendorId, 0xffff, "vendorId");
        this.productId = ManagedDescriptorSupport.unsigned(productId, 0xffff, "productId");
        this.deviceReleaseBcd = ManagedDescriptorSupport.unsigned(
                deviceReleaseBcd, 0xffff, "deviceReleaseBcd");
        this.configurations = ManagedDescriptorSupport.immutableList(
                configurations, "configurations");
        for (GenericUsbConfiguration configuration : this.configurations) {
            if (configuration.getSnapshotGeneration() != snapshotGeneration) {
                throw new IllegalArgumentException("configuration generation mismatch");
            }
        }
        this.snapshotGeneration = ManagedDescriptorSupport.generation(snapshotGeneration);
    }

    public int getUsbVersionBcd() { return usbVersionBcd; }
    public int getDeviceClass() { return deviceClass; }
    public int getDeviceSubclass() { return deviceSubclass; }
    public int getDeviceProtocol() { return deviceProtocol; }
    public int getEndpointZeroMaximumPacketSize() { return endpointZeroMaximumPacketSize; }
    public int getVendorId() { return vendorId; }
    public int getProductId() { return productId; }
    public int getDeviceReleaseBcd() { return deviceReleaseBcd; }
    public int getConfigurationCount() { return configurations.size(); }
    public List<GenericUsbConfiguration> getConfigurations() { return configurations; }
    long getSnapshotGeneration() { return snapshotGeneration; }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof GenericUsbDeviceDescriptor)) return false;
        GenericUsbDeviceDescriptor that = (GenericUsbDeviceDescriptor) other;
        return usbVersionBcd == that.usbVersionBcd && deviceClass == that.deviceClass
                && deviceSubclass == that.deviceSubclass && deviceProtocol == that.deviceProtocol
                && endpointZeroMaximumPacketSize == that.endpointZeroMaximumPacketSize
                && vendorId == that.vendorId && productId == that.productId
                && deviceReleaseBcd == that.deviceReleaseBcd
                && snapshotGeneration == that.snapshotGeneration
                && configurations.equals(that.configurations);
    }
    @Override public int hashCode() {
        return Objects.hash(usbVersionBcd, deviceClass, deviceSubclass, deviceProtocol,
                endpointZeroMaximumPacketSize, vendorId, productId, deviceReleaseBcd,
                configurations, snapshotGeneration);
    }
}

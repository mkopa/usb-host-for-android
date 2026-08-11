package info.marcin.usbhost.transport;

import java.util.List;
import java.util.Objects;

/** Immutable USB interface alternate-setting descriptor. */
public final class GenericUsbAlternateSetting {
    private final int interfaceNumber;
    private final int alternateSetting;
    private final int interfaceClass;
    private final int interfaceSubclass;
    private final int interfaceProtocol;
    private final List<GenericUsbEndpoint> endpoints;
    private final List<AdditionalUsbDescriptor> additionalDescriptors;
    private final long snapshotGeneration;

    GenericUsbAlternateSetting(int interfaceNumber, int alternateSetting, int interfaceClass,
            int interfaceSubclass, int interfaceProtocol, List<GenericUsbEndpoint> endpoints,
            List<AdditionalUsbDescriptor> additionalDescriptors, long snapshotGeneration) {
        this.interfaceNumber = ManagedDescriptorSupport.unsigned(
                interfaceNumber, 0xff, "interfaceNumber");
        this.alternateSetting = ManagedDescriptorSupport.unsigned(
                alternateSetting, 0xff, "alternateSetting");
        this.interfaceClass = ManagedDescriptorSupport.unsigned(interfaceClass, 0xff, "class");
        this.interfaceSubclass = ManagedDescriptorSupport.unsigned(
                interfaceSubclass, 0xff, "subclass");
        this.interfaceProtocol = ManagedDescriptorSupport.unsigned(
                interfaceProtocol, 0xff, "protocol");
        this.endpoints = ManagedDescriptorSupport.immutableList(endpoints, "endpoints");
        for (GenericUsbEndpoint endpoint : this.endpoints) {
            if (endpoint.getSnapshotGeneration() != snapshotGeneration) {
                throw new IllegalArgumentException("endpoint generation mismatch");
            }
        }
        this.additionalDescriptors = ManagedDescriptorSupport.immutableList(
                additionalDescriptors, "additionalDescriptors");
        this.snapshotGeneration = ManagedDescriptorSupport.generation(snapshotGeneration);
    }

    public int getInterfaceNumber() { return interfaceNumber; }
    public int getAlternateSetting() { return alternateSetting; }
    public int getInterfaceClass() { return interfaceClass; }
    public int getInterfaceSubclass() { return interfaceSubclass; }
    public int getInterfaceProtocol() { return interfaceProtocol; }
    public List<GenericUsbEndpoint> getEndpoints() { return endpoints; }
    public List<AdditionalUsbDescriptor> getAdditionalDescriptors() { return additionalDescriptors; }
    long getSnapshotGeneration() { return snapshotGeneration; }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof GenericUsbAlternateSetting)) return false;
        GenericUsbAlternateSetting that = (GenericUsbAlternateSetting) other;
        return interfaceNumber == that.interfaceNumber && alternateSetting == that.alternateSetting
                && interfaceClass == that.interfaceClass && interfaceSubclass == that.interfaceSubclass
                && interfaceProtocol == that.interfaceProtocol
                && snapshotGeneration == that.snapshotGeneration && endpoints.equals(that.endpoints)
                && additionalDescriptors.equals(that.additionalDescriptors);
    }
    @Override public int hashCode() {
        return Objects.hash(interfaceNumber, alternateSetting, interfaceClass, interfaceSubclass,
                interfaceProtocol, endpoints, additionalDescriptors, snapshotGeneration);
    }
}

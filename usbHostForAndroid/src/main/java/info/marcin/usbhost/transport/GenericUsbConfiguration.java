package info.marcin.usbhost.transport;

import java.util.List;
import java.util.Objects;

/** Immutable USB configuration snapshot. */
public final class GenericUsbConfiguration {
    private final int configurationValue;
    private final int attributes;
    private final int maximumPower;
    private final boolean active;
    private final List<GenericUsbInterfaceDescriptor> interfaces;
    private final List<AdditionalUsbDescriptor> additionalDescriptors;
    private final long snapshotGeneration;

    GenericUsbConfiguration(int configurationValue, int attributes, int maximumPower,
            boolean active, List<GenericUsbInterfaceDescriptor> interfaces,
            List<AdditionalUsbDescriptor> additionalDescriptors, long snapshotGeneration) {
        this.configurationValue = ManagedDescriptorSupport.unsigned(
                configurationValue, 0xff, "configurationValue");
        if (configurationValue == 0) throw new IllegalArgumentException("configurationValue is zero");
        this.attributes = ManagedDescriptorSupport.unsigned(attributes, 0xff, "attributes");
        this.maximumPower = ManagedDescriptorSupport.unsigned(maximumPower, 0xff, "maximumPower");
        this.active = active;
        this.interfaces = ManagedDescriptorSupport.immutableList(interfaces, "interfaces");
        for (GenericUsbInterfaceDescriptor interfaceDescriptor : this.interfaces) {
            if (interfaceDescriptor.getSnapshotGeneration() != snapshotGeneration) {
                throw new IllegalArgumentException("interface generation mismatch");
            }
        }
        this.additionalDescriptors = ManagedDescriptorSupport.immutableList(
                additionalDescriptors, "additionalDescriptors");
        this.snapshotGeneration = ManagedDescriptorSupport.generation(snapshotGeneration);
    }

    public int getConfigurationValue() { return configurationValue; }
    public int getAttributes() { return attributes; }
    public int getMaximumPower() { return maximumPower; }
    public boolean isActive() { return active; }
    public List<GenericUsbInterfaceDescriptor> getInterfaces() { return interfaces; }
    public List<AdditionalUsbDescriptor> getAdditionalDescriptors() { return additionalDescriptors; }
    long getSnapshotGeneration() { return snapshotGeneration; }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof GenericUsbConfiguration)) return false;
        GenericUsbConfiguration that = (GenericUsbConfiguration) other;
        return configurationValue == that.configurationValue && attributes == that.attributes
                && maximumPower == that.maximumPower && active == that.active
                && snapshotGeneration == that.snapshotGeneration && interfaces.equals(that.interfaces)
                && additionalDescriptors.equals(that.additionalDescriptors);
    }
    @Override public int hashCode() {
        return Objects.hash(configurationValue, attributes, maximumPower, active, interfaces,
                additionalDescriptors, snapshotGeneration);
    }
}

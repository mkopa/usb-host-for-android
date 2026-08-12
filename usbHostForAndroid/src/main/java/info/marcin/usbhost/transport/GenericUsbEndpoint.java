package info.marcin.usbhost.transport;

import java.util.List;
import java.util.Objects;

/** Immutable endpoint descriptor bound internally to one session snapshot generation. */
public final class GenericUsbEndpoint {
    private final int address;
    private final int endpointNumber;
    private final UsbDirection direction;
    private final UsbTransferType transferType;
    private final int maximumPacketSize;
    private final int interval;
    private final List<AdditionalUsbDescriptor> additionalDescriptors;
    private final long snapshotGeneration;
    private final Object ownerToken;

    GenericUsbEndpoint(int address, int endpointNumber, UsbDirection direction,
            UsbTransferType transferType, int maximumPacketSize, int interval,
            List<AdditionalUsbDescriptor> additionalDescriptors, long snapshotGeneration) {
        this(address, endpointNumber, direction, transferType, maximumPacketSize, interval,
                additionalDescriptors, snapshotGeneration, null);
    }

    GenericUsbEndpoint(int address, int endpointNumber, UsbDirection direction,
            UsbTransferType transferType, int maximumPacketSize, int interval,
            List<AdditionalUsbDescriptor> additionalDescriptors, long snapshotGeneration,
            Object ownerToken) {
        this.address = ManagedDescriptorSupport.unsigned(address, 0xff, "address");
        this.endpointNumber = ManagedDescriptorSupport.unsigned(
                endpointNumber, 0x0f, "endpointNumber");
        if (endpointNumber == 0 || (address & 0x0f) != endpointNumber) {
            throw new IllegalArgumentException("endpoint number must match the non-zero address");
        }
        this.direction = Objects.requireNonNull(direction, "direction");
        UsbDirection addressDirection = (address & 0x80) == 0 ? UsbDirection.OUT : UsbDirection.IN;
        if (direction != addressDirection) throw new IllegalArgumentException("direction mismatch");
        this.transferType = Objects.requireNonNull(transferType, "transferType");
        this.maximumPacketSize = ManagedDescriptorSupport.unsigned(
                maximumPacketSize, 0xffff, "maximumPacketSize");
        this.interval = ManagedDescriptorSupport.unsigned(interval, 0xff, "interval");
        this.additionalDescriptors = ManagedDescriptorSupport.immutableList(
                additionalDescriptors, "additionalDescriptors");
        this.snapshotGeneration = ManagedDescriptorSupport.generation(snapshotGeneration);
        this.ownerToken = ownerToken;
    }

    public int getAddress() { return address; }
    public int getEndpointNumber() { return endpointNumber; }
    public UsbDirection getDirection() { return direction; }
    public UsbTransferType getTransferType() { return transferType; }
    public int getMaximumPacketSize() { return maximumPacketSize; }
    public int getInterval() { return interval; }
    public List<AdditionalUsbDescriptor> getAdditionalDescriptors() { return additionalDescriptors; }
    long getSnapshotGeneration() { return snapshotGeneration; }
    Object getOwnerToken() { return ownerToken; }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof GenericUsbEndpoint)) return false;
        GenericUsbEndpoint that = (GenericUsbEndpoint) other;
        return address == that.address && endpointNumber == that.endpointNumber
                && maximumPacketSize == that.maximumPacketSize && interval == that.interval
                && snapshotGeneration == that.snapshotGeneration && direction == that.direction
                && transferType == that.transferType
                && additionalDescriptors.equals(that.additionalDescriptors);
    }
    @Override public int hashCode() {
        return Objects.hash(address, endpointNumber, direction, transferType, maximumPacketSize,
                interval, additionalDescriptors, snapshotGeneration);
    }
}

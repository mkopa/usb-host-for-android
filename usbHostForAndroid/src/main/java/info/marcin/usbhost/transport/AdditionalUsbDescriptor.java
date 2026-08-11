package info.marcin.usbhost.transport;

import java.util.Arrays;
import java.util.Objects;

/** Immutable copy of an additional USB descriptor, including its original header. */
public final class AdditionalUsbDescriptor {
    private final int type;
    private final byte[] bytes;

    AdditionalUsbDescriptor(int type, byte[] bytes) {
        this.type = ManagedDescriptorSupport.unsigned(type, 0xff, "type");
        Objects.requireNonNull(bytes, "bytes");
        if (bytes.length < 2 || bytes.length > 0xffff || (bytes[0] & 0xff) != bytes.length
                || (bytes[1] & 0xff) != type) {
            throw new IllegalArgumentException("raw descriptor header does not match its value");
        }
        this.bytes = bytes.clone();
    }

    public int getType() { return type; }
    public byte[] getBytes() { return bytes.clone(); }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof AdditionalUsbDescriptor)) return false;
        AdditionalUsbDescriptor that = (AdditionalUsbDescriptor) other;
        return type == that.type && Arrays.equals(bytes, that.bytes);
    }
    @Override public int hashCode() { return 31 * Integer.hashCode(type) + Arrays.hashCode(bytes); }
}

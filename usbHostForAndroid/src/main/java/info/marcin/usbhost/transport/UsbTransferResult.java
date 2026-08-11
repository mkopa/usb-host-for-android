package info.marcin.usbhost.transport;

import java.util.Objects;

/** Immutable stable status and exact byte count returned by a USB transfer. */
public final class UsbTransferResult {
    private static final int MAXIMUM_TRANSFER_LENGTH = 1_048_576;

    private final UsbTransportStatus status;
    private final int actualLength;

    public UsbTransferResult(UsbTransportStatus status, int actualLength) {
        this.status = Objects.requireNonNull(status, "status");
        this.actualLength = requireActualLength(actualLength);
    }

    public UsbTransportStatus getStatus() {
        return status;
    }

    public int getActualLength() {
        return actualLength;
    }

    public boolean isSuccessful() {
        return status == UsbTransportStatus.OK;
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof UsbTransferResult)) {
            return false;
        }
        UsbTransferResult that = (UsbTransferResult) other;
        return status == that.status && actualLength == that.actualLength;
    }

    @Override
    public int hashCode() {
        return Objects.hash(status, actualLength);
    }

    static int requireActualLength(int candidate) {
        if (candidate < 0 || candidate > MAXIMUM_TRANSFER_LENGTH) {
            throw new IllegalArgumentException(
                    "actualLength must be between 0 and " + MAXIMUM_TRANSFER_LENGTH);
        }
        return candidate;
    }
}

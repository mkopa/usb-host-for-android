package info.marcin.usbhost.transport;

import java.util.Objects;

/** Checked USB failure with a stable status and an optional partial byte count. */
public final class UsbTransportException extends Exception {
    private static final long serialVersionUID = 1L;
    private static final int MAXIMUM_DIAGNOSTIC_LENGTH = 240;

    private final UsbTransportStatus status;
    private final int actualLength;

    public UsbTransportException(UsbTransportStatus status, String message) {
        this(status, 0, message);
    }

    public UsbTransportException(UsbTransportStatus status, int actualLength, String message) {
        super(sanitizeMessage(status, message));
        this.status = requireFailure(status);
        this.actualLength = UsbTransferResult.requireActualLength(actualLength);
    }

    public UsbTransportStatus getStatus() {
        return status;
    }

    public int getActualLength() {
        return actualLength;
    }

    private static UsbTransportStatus requireFailure(UsbTransportStatus status) {
        Objects.requireNonNull(status, "status");
        if (status == UsbTransportStatus.OK) {
            throw new IllegalArgumentException("An exception cannot carry OK status");
        }
        return status;
    }

    private static String sanitizeMessage(UsbTransportStatus status, String message) {
        requireFailure(status);
        if (message == null || message.isEmpty()) {
            return status.name();
        }
        StringBuilder sanitized = new StringBuilder(
                Math.min(message.length(), MAXIMUM_DIAGNOSTIC_LENGTH));
        for (int index = 0;
                index < message.length() && sanitized.length() < MAXIMUM_DIAGNOSTIC_LENGTH;
                ++index) {
            char character = message.charAt(index);
            sanitized.append(Character.isISOControl(character) ? '?' : character);
        }
        return sanitized.toString();
    }
}

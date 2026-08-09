package dev.usbhost.android;

/** Checked failure returned by an ST-Link operation. */
public final class StlinkException extends Exception {
    private final StlinkStatus status;

    public StlinkException(StlinkStatus status, String message) {
        super(message == null || message.isEmpty() ? status.name() : message);
        this.status = status;
    }

    public StlinkStatus getStatus() {
        return status;
    }
}

package info.marcin.usbhost.transport;

/** Stable status values shared with the public native C ABI. */
public enum UsbTransportStatus {
    OK(0),
    INVALID_ARGUMENT(1),
    PERMISSION_DENIED(2),
    UNSUPPORTED_DEVICE(3),
    USB_ERROR(4),
    TIMEOUT(5),
    DISCONNECTED(6),
    PROGRAMMER_ERROR(7),
    TARGET_NOT_FOUND(8),
    UNSUPPORTED_TARGET(9),
    INVALID_STATE(10),
    BUSY(11),
    INTERNAL_ERROR(12),
    STALL(13),
    CANCELLED(14),
    UNSUPPORTED_OPERATION(15);

    private final int code;

    UsbTransportStatus(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    public static UsbTransportStatus fromCode(int code) {
        for (UsbTransportStatus status : values()) {
            if (status.code == code) {
                return status;
            }
        }
        return INTERNAL_ERROR;
    }
}

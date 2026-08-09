package dev.usbhost.android;

/** Stable status values shared with the native C ABI. */
public enum StlinkStatus {
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
    INTERNAL_ERROR(12);

    private final int code;

    StlinkStatus(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    static StlinkStatus fromCode(int code) {
        for (StlinkStatus status : values()) {
            if (status.code == code) {
                return status;
            }
        }
        return INTERNAL_ERROR;
    }
}

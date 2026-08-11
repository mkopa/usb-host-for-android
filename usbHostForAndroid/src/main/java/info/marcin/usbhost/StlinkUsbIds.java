package info.marcin.usbhost;

/** ST-Link USB identifiers supported by this release. */
public final class StlinkUsbIds {
    public static final int ST_VENDOR_ID = 0x0483;
    public static final int STLINK_V3E = 0x374e;
    public static final int STLINK_V3S = 0x374f;
    public static final int STLINK_V3_2VCP = 0x3753;
    public static final int STLINK_V3_NO_MSD = 0x3754;
    public static final int STLINK_V3P = 0x3757;

    private StlinkUsbIds() {}

    public static boolean isSupported(int vendorId, int productId) {
        if (vendorId != ST_VENDOR_ID) {
            return false;
        }
        return productId == STLINK_V3E
                || productId == STLINK_V3S
                || productId == STLINK_V3_2VCP
                || productId == STLINK_V3_NO_MSD
                || productId == STLINK_V3P;
    }
}

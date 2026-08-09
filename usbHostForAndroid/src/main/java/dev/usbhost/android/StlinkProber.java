package dev.usbhost.android;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

/** Discovery helpers that operate only on devices already enumerated by Android. */
public final class StlinkProber {
    private StlinkProber() {}

    public static boolean isSupported(UsbDevice device) {
        return device != null
                && StlinkUsbIds.isSupported(device.getVendorId(), device.getProductId());
    }

    public static List<StlinkDevice> findAll(UsbManager manager) {
        Objects.requireNonNull(manager, "manager");
        List<StlinkDevice> result = new ArrayList<>();
        for (UsbDevice device : manager.getDeviceList().values()) {
            if (isSupported(device)) {
                result.add(new StlinkDevice(device));
            }
        }
        Collections.sort(result, (left, right) -> compareIdentity(
                left.getDeviceId(), left.getDeviceName(),
                right.getDeviceId(), right.getDeviceName()));
        return Collections.unmodifiableList(result);
    }

    static int compareIdentity(int leftId, String leftName, int rightId, String rightName) {
        int byId = Integer.compare(leftId, rightId);
        if (byId != 0) {
            return byId;
        }
        if (leftName == null) {
            return rightName == null ? 0 : -1;
        }
        return rightName == null ? 1 : leftName.compareTo(rightName);
    }
}

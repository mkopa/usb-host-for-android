package info.marcin.usbhost.consumer.smoke;

import android.hardware.usb.UsbManager;

import java.util.List;

import info.marcin.usbhost.StlinkDevice;
import info.marcin.usbhost.StlinkProber;

/** Compile-only proof that a detached Android project can consume the public 0.1.0 API. */
public final class ConsumerProbe {
    private ConsumerProbe() {}

    public static List<StlinkDevice> scan(UsbManager manager) {
        return StlinkProber.findAll(manager);
    }
}

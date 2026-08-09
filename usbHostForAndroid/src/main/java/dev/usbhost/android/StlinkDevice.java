package dev.usbhost.android;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;

import java.util.Objects;

/** Immutable descriptor for one currently attached supported ST-Link V3. */
public final class StlinkDevice {
    public static final int DEFAULT_SWD_FREQUENCY_KHZ = 1800;

    private final UsbDevice usbDevice;

    public StlinkDevice(UsbDevice usbDevice) {
        this.usbDevice = Objects.requireNonNull(usbDevice, "usbDevice");
    }

    public UsbDevice getUsbDevice() { return usbDevice; }
    public int getDeviceId() { return usbDevice.getDeviceId(); }
    public String getDeviceName() { return usbDevice.getDeviceName(); }
    public int getVendorId() { return usbDevice.getVendorId(); }
    public int getProductId() { return usbDevice.getProductId(); }

    public StlinkSession open(UsbManager manager) throws StlinkException {
        return open(manager, DEFAULT_SWD_FREQUENCY_KHZ);
    }

    public StlinkSession open(UsbManager manager, int swdFrequencyKhz) throws StlinkException {
        Objects.requireNonNull(manager, "manager");
        StlinkSession.requireWorkerThread();
        if (!StlinkUsbIds.isSupported(getVendorId(), getProductId())) {
            throw new StlinkException(StlinkStatus.UNSUPPORTED_DEVICE,
                    "USB device is not a supported ST-Link V3 debug interface");
        }
        if (!manager.hasPermission(usbDevice)) {
            throw new StlinkException(StlinkStatus.PERMISSION_DENIED,
                    "Android USB permission has not been granted");
        }
        if (swdFrequencyKhz <= 0 || swdFrequencyKhz > 24000) {
            throw new StlinkException(StlinkStatus.INVALID_ARGUMENT,
                    "SWD frequency must be between 1 and 24000 kHz");
        }

        UsbDeviceConnection connection = manager.openDevice(usbDevice);
        if (connection == null) {
            throw new StlinkException(StlinkStatus.USB_ERROR,
                    "Android could not open the USB device");
        }
        try {
            long[] opened = NativeBridge.open(connection.getFileDescriptor(), getVendorId(),
                    getProductId(), swdFrequencyKhz);
            if (opened == null || opened.length != 6) {
                throw new StlinkException(StlinkStatus.INTERNAL_ERROR,
                        "Native open returned an invalid result");
            }
            StlinkStatus status = StlinkStatus.fromCode((int) opened[0]);
            if (status != StlinkStatus.OK || opened[1] == 0) {
                throw new StlinkException(status, NativeBridge.lastError());
            }
            ProgrammerInfo info = new ProgrammerInfo(
                    (int) opened[2], (int) opened[3], (int) opened[4], (int) opened[5]);
            return new StlinkSession(connection, opened[1], info);
        } catch (StlinkException | RuntimeException error) {
            connection.close();
            throw error;
        }
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof StlinkDevice)) return false;
        StlinkDevice that = (StlinkDevice) other;
        return getDeviceId() == that.getDeviceId()
                && getVendorId() == that.getVendorId()
                && getProductId() == that.getProductId()
                && Objects.equals(getDeviceName(), that.getDeviceName());
    }

    @Override
    public int hashCode() {
        return Objects.hash(getDeviceId(), getDeviceName(), getVendorId(), getProductId());
    }
}

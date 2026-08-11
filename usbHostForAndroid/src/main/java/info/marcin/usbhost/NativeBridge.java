package info.marcin.usbhost;

final class NativeBridge {
    static {
        System.loadLibrary("usbhost");
    }

    private NativeBridge() {}

    static native long[] open(int fd, int vendorId, int productId, int swdFrequencyKhz);
    static native long[] connectTarget(long handle);
    static native byte[] readMemory(long handle, long address, int length);
    static native int close(long handle);
    static native int lastStatus();
    static native String lastError();
}

package info.marcin.usbhost.transport;

/** Package-private primitive-only JNI boundary. It never owns Android USB objects. */
final class TransportNativeBridge {
    static final int OPEN_RECORD_LENGTH = 2;
    static final int DEVICE_RECORD_LENGTH = 11;
    static final int CONFIGURATION_RECORD_LENGTH = 9;
    static final int INTERFACE_RECORD_LENGTH = 7;
    static final int ALTERNATE_RECORD_LENGTH = 10;
    static final int ENDPOINT_RECORD_LENGTH = 10;
    static final int TRANSFER_RECORD_LENGTH = 2;

    static {
        System.loadLibrary("usbhost");
    }

    private TransportNativeBridge() {}

    static native long[] openSession(int authorizedFileDescriptor);
    static native long[] getDeviceDescriptor(long session);
    static native long[] getConfiguration(long session, int configurationIndex);
    static native long[] getInterface(long session, int configurationIndex, int interfaceIndex);
    static native long[] getAlternateSetting(long session, int configurationIndex,
            int interfaceIndex, int alternateSettingIndex);
    static native long[] getEndpoint(long session, int configurationIndex, int interfaceIndex,
            int alternateSettingIndex, int endpointIndex);
    static native byte[] getAdditionalDescriptor(long session, int scope,
            long snapshotGeneration, int configurationIndex, int interfaceIndex,
            int alternateSettingIndex, int endpointIndex, int additionalDescriptorIndex);
    static native int selectConfiguration(long session, int configurationValue);
    static native int claimInterface(long session, int interfaceNumber);
    static native int selectAlternateSetting(long session, int interfaceNumber,
            int alternateSetting);
    static native int releaseInterface(long session, int interfaceNumber);
    static native long[] controlTransfer(long session, int requestType, int request,
            int value, int index, byte[] buffer, int offset, int length, int timeoutMillis);
    static native long[] bulkTransfer(long session, int endpointAddress,
            byte[] buffer, int offset, int length, int timeoutMillis);
    static native long[] interruptTransfer(long session, int endpointAddress,
            byte[] buffer, int offset, int length, int timeoutMillis);
    static native int cancel(long session);
    static native int close(long session);
    static native String lastError();
}

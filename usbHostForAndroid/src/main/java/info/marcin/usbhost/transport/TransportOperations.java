package info.marcin.usbhost.transport;

interface TransportOperations {
    long[] openSession(int fd);
    long[] getDeviceDescriptor(long session);
    long[] getConfiguration(long session, int configurationIndex);
    long[] getInterface(long session, int configurationIndex, int interfaceIndex);
    long[] getAlternateSetting(long session, int configurationIndex, int interfaceIndex,
            int alternateSettingIndex);
    long[] getEndpoint(long session, int configurationIndex, int interfaceIndex,
            int alternateSettingIndex, int endpointIndex);
    byte[] getAdditionalDescriptor(long session, int scope, long generation,
            int configurationIndex, int interfaceIndex, int alternateSettingIndex,
            int endpointIndex, int additionalDescriptorIndex);
    int selectConfiguration(long session, int value);
    int claimInterface(long session, int value);
    int selectAlternateSetting(long session, int number, int alternate);
    int releaseInterface(long session, int value);
    int cancel(long session);
    int close(long session);
    String lastError();

    TransportOperations NATIVE = new TransportOperations() {
        @Override public long[] openSession(int fd) {
            return TransportNativeBridge.openSession(fd);
        }
        @Override public long[] getDeviceDescriptor(long session) {
            return TransportNativeBridge.getDeviceDescriptor(session);
        }
        @Override public long[] getConfiguration(long session, int index) {
            return TransportNativeBridge.getConfiguration(session, index);
        }
        @Override public long[] getInterface(long session, int configuration, int index) {
            return TransportNativeBridge.getInterface(session, configuration, index);
        }
        @Override public long[] getAlternateSetting(
                long session, int configuration, int iface, int index) {
            return TransportNativeBridge.getAlternateSetting(session, configuration, iface, index);
        }
        @Override public long[] getEndpoint(
                long session, int configuration, int iface, int alternate, int index) {
            return TransportNativeBridge.getEndpoint(
                    session, configuration, iface, alternate, index);
        }
        @Override public byte[] getAdditionalDescriptor(long session, int scope, long generation,
                int configuration, int iface, int alternate, int endpoint, int index) {
            return TransportNativeBridge.getAdditionalDescriptor(session, scope, generation,
                    configuration, iface, alternate, endpoint, index);
        }
        @Override public int selectConfiguration(long session, int value) {
            return TransportNativeBridge.selectConfiguration(session, value);
        }
        @Override public int claimInterface(long session, int value) {
            return TransportNativeBridge.claimInterface(session, value);
        }
        @Override public int selectAlternateSetting(long session, int number, int alternate) {
            return TransportNativeBridge.selectAlternateSetting(session, number, alternate);
        }
        @Override public int releaseInterface(long session, int value) {
            return TransportNativeBridge.releaseInterface(session, value);
        }
        @Override public int cancel(long session) { return TransportNativeBridge.cancel(session); }
        @Override public int close(long session) { return TransportNativeBridge.close(session); }
        @Override public String lastError() { return TransportNativeBridge.lastError(); }
    };
}

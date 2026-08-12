package info.marcin.usbhost.transport;

/** Exclusive claimed-interface token bound to one managed device session. */
public final class GenericUsbInterface implements AutoCloseable {
    private final Object stateLock = new Object();
    private final GenericUsbDevice parent;
    private final int interfaceNumber;
    private GenericUsbAlternateSetting activeAlternateSetting;
    private boolean claimed = true;

    GenericUsbInterface(GenericUsbDevice parent, GenericUsbInterfaceDescriptor descriptor)
            throws UsbTransportException {
        this.parent = parent;
        this.interfaceNumber = descriptor.getInterfaceNumber();
        this.activeAlternateSetting = findAlternate(
                descriptor, descriptor.getActiveAlternateSetting());
        if (activeAlternateSetting == null) throw GenericUsbDevice.failure(
                UsbTransportStatus.INTERNAL_ERROR, "Active alternate setting is unavailable");
    }

    public int getInterfaceNumber() { return interfaceNumber; }

    public GenericUsbAlternateSetting getActiveAlternateSetting() {
        synchronized (stateLock) { return activeAlternateSetting; }
    }

    public UsbTransferResult bulkTransfer(GenericUsbEndpoint endpoint, byte[] buffer,
            int offset, int length, int timeoutMillis) throws UsbTransportException {
        return bulkTransferForTesting(
                endpoint, buffer, offset, length, timeoutMillis, GenericUsbDevice.isMainThread());
    }

    UsbTransferResult bulkTransferForTesting(GenericUsbEndpoint endpoint, byte[] buffer,
            int offset, int length, int timeoutMillis, boolean mainThread)
            throws UsbTransportException {
        return endpointTransfer(endpoint, buffer, offset, length, timeoutMillis,
                mainThread, UsbTransferType.BULK);
    }

    public UsbTransferResult interruptTransfer(GenericUsbEndpoint endpoint, byte[] buffer,
            int offset, int length, int timeoutMillis) throws UsbTransportException {
        return interruptTransferForTesting(
                endpoint, buffer, offset, length, timeoutMillis, GenericUsbDevice.isMainThread());
    }

    UsbTransferResult interruptTransferForTesting(GenericUsbEndpoint endpoint, byte[] buffer,
            int offset, int length, int timeoutMillis, boolean mainThread)
            throws UsbTransportException {
        return endpointTransfer(endpoint, buffer, offset, length, timeoutMillis,
                mainThread, UsbTransferType.INTERRUPT);
    }

    private UsbTransferResult endpointTransfer(GenericUsbEndpoint endpoint, byte[] buffer,
            int offset, int length, int timeoutMillis, boolean mainThread,
            UsbTransferType expectedType) throws UsbTransportException {
        GenericUsbDevice.requireWorkerThread(mainThread);
        validateEndpoint(endpoint);
        if (endpoint.getTransferType() != expectedType) throw GenericUsbDevice.failure(
                UsbTransportStatus.INVALID_ARGUMENT,
                "Endpoint transfer type does not match the requested operation");
        GenericUsbDevice.validateTransferArguments(
                buffer, offset, length, timeoutMillis, 1_048_576);
        long handle = parent.requireOpenHandle();
        long[] result = expectedType == UsbTransferType.BULK
                ? parent.operations().bulkTransfer(handle, endpoint.getAddress(),
                        buffer, offset, length, timeoutMillis)
                : parent.operations().interruptTransfer(handle, endpoint.getAddress(),
                        buffer, offset, length, timeoutMillis);
        return GenericUsbDevice.requireTransferResult(result, length, parent.operations());
    }

    public void selectAlternateSetting(int alternateSetting) throws UsbTransportException {
        GenericUsbDevice.requireWorkerThread(GenericUsbDevice.isMainThread());
        if (alternateSetting < 0 || alternateSetting > 0xff) throw GenericUsbDevice.failure(
                UsbTransportStatus.INVALID_ARGUMENT,
                "alternateSetting is outside its unsigned range");
        requireClaimed();
        long handle = parent.requireOpenHandle();
        GenericUsbDevice.checkStatus(parent.operations().selectAlternateSetting(
                handle, interfaceNumber, alternateSetting), parent.operations());
        parent.refreshDescriptor(handle);
        GenericUsbInterfaceDescriptor refreshed = parent.findInterface(interfaceNumber);
        GenericUsbAlternateSetting active = refreshed == null
                ? null : findAlternate(refreshed, alternateSetting);
        if (active == null) throw GenericUsbDevice.failure(
                UsbTransportStatus.INTERNAL_ERROR,
                "Selected alternate setting is absent from the native snapshot");
        synchronized (stateLock) {
            if (!claimed) throw GenericUsbDevice.failure(
                    UsbTransportStatus.INVALID_STATE, "Interface was released during selection");
            activeAlternateSetting = active;
        }
    }

    public boolean isClaimed() {
        synchronized (stateLock) { return claimed && parent.isOpen(); }
    }

    @Override
    public void close() throws UsbTransportException {
        GenericUsbDevice.requireWorkerThread(GenericUsbDevice.isMainThread());
        synchronized (stateLock) {
            if (!claimed) return;
            claimed = false;
        }
        if (!parent.isOpen()) return;
        long handle = parent.requireOpenHandle();
        int status = parent.operations().releaseInterface(handle, interfaceNumber);
        if (UsbTransportStatus.fromCode(status) != UsbTransportStatus.OK) {
            synchronized (stateLock) { claimed = true; }
            GenericUsbDevice.checkStatus(status, parent.operations());
        }
        parent.refreshDescriptor(handle);
    }

    void validateEndpoint(GenericUsbEndpoint endpoint) throws UsbTransportException {
        if (endpoint == null) throw GenericUsbDevice.failure(
                UsbTransportStatus.INVALID_ARGUMENT, "endpoint is null");
        requireClaimed();
        GenericUsbAlternateSetting active = getActiveAlternateSetting();
        if (endpoint.getOwnerToken() != parent.ownerToken()
                || endpoint.getSnapshotGeneration() != active.getSnapshotGeneration()) {
            throw GenericUsbDevice.failure(
                    UsbTransportStatus.INVALID_STATE, "Endpoint is foreign or stale");
        }
        for (GenericUsbEndpoint candidate : active.getEndpoints()) {
            if (candidate == endpoint || candidate.getAddress() == endpoint.getAddress()) return;
        }
        throw GenericUsbDevice.failure(
                UsbTransportStatus.INVALID_STATE, "Endpoint is not active on this interface");
    }

    private void requireClaimed() throws UsbTransportException {
        if (!isClaimed()) throw GenericUsbDevice.failure(
                UsbTransportStatus.INVALID_STATE, "Interface is released");
    }

    private static GenericUsbAlternateSetting findAlternate(
            GenericUsbInterfaceDescriptor descriptor, int alternateSetting) {
        for (GenericUsbAlternateSetting value : descriptor.getAlternateSettings()) {
            if (value.getAlternateSetting() == alternateSetting) return value;
        }
        return null;
    }
}

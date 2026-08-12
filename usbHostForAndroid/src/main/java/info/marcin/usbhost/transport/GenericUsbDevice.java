package info.marcin.usbhost.transport;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.os.Looper;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

/** Permission-safe generic USB session opened from a caller-owned Android connection. */
public final class GenericUsbDevice implements AutoCloseable {
    interface FileDescriptorSource { int getFileDescriptor(); }

    private final Object stateLock = new Object();
    private final TransportOperations operations;
    private final Object ownerToken;
    private long session;
    private GenericUsbDeviceDescriptor descriptor;

    private GenericUsbDevice(long session, TransportOperations operations, Object ownerToken,
            GenericUsbDeviceDescriptor descriptor) {
        this.session = session;
        this.operations = operations;
        this.ownerToken = ownerToken;
        this.descriptor = descriptor;
    }

    public static GenericUsbDevice open(UsbDevice device, UsbDeviceConnection connection)
            throws UsbTransportException {
        Objects.requireNonNull(device, "device");
        Objects.requireNonNull(connection, "connection");
        return openForTesting(connection::getFileDescriptor, device.getVendorId(),
                device.getProductId(), TransportOperations.NATIVE, isMainThread());
    }

    static GenericUsbDevice openForTesting(FileDescriptorSource source, int expectedVendorId,
            int expectedProductId, TransportOperations operations, boolean mainThread)
            throws UsbTransportException {
        Objects.requireNonNull(source, "source");
        Objects.requireNonNull(operations, "operations");
        requireWorkerThread(mainThread);
        requireUnsigned(expectedVendorId, 0xffff, "vendorId");
        requireUnsigned(expectedProductId, 0xffff, "productId");
        int fd = source.getFileDescriptor();
        if (fd < 0) throw failure(UsbTransportStatus.INVALID_STATE, "USB connection is closed");
        long[] opened = operations.openSession(fd);
        requireRecord(opened, TransportNativeBridge.OPEN_RECORD_LENGTH, operations);
        checkStatus((int) opened[0], operations);
        if (opened[1] == 0) throw failure(
                UsbTransportStatus.INTERNAL_ERROR, "Native open returned an invalid session");
        long session = opened[1];
        Object ownerToken = new Object();
        try {
            GenericUsbDeviceDescriptor descriptor = readDescriptor(session, operations, ownerToken);
            if (descriptor.getVendorId() != expectedVendorId
                    || descriptor.getProductId() != expectedProductId) {
                throw failure(UsbTransportStatus.UNSUPPORTED_DEVICE,
                        "Authorized connection does not match the requested USB device");
            }
            return new GenericUsbDevice(session, operations, ownerToken, descriptor);
        } catch (UsbTransportException | RuntimeException error) {
            operations.close(session);
            throw error;
        }
    }

    public GenericUsbDeviceDescriptor getDescriptor() {
        synchronized (stateLock) { return descriptor; }
    }

    public List<GenericUsbConfiguration> getConfigurations() {
        return getDescriptor().getConfigurations();
    }

    public GenericUsbConfiguration getActiveConfiguration() {
        for (GenericUsbConfiguration configuration : getConfigurations()) {
            if (configuration.isActive()) return configuration;
        }
        return null;
    }

    public void selectConfiguration(int configurationValue) throws UsbTransportException {
        requireWorkerThread(isMainThread());
        requireUnsigned(configurationValue, 0xff, "configurationValue");
        if (configurationValue == 0) throw failure(
                UsbTransportStatus.INVALID_ARGUMENT, "Configuration value cannot be zero");
        long handle = requireOpenHandle();
        checkStatus(operations.selectConfiguration(handle, configurationValue), operations);
        GenericUsbDeviceDescriptor refreshed = readDescriptor(handle, operations, ownerToken);
        synchronized (stateLock) {
            if (session != handle) throw failure(
                    UsbTransportStatus.INVALID_STATE, "Session closed during configuration change");
            descriptor = refreshed;
        }
    }

    public void cancelActiveTransfer() throws UsbTransportException {
        requireWorkerThread(isMainThread());
        checkStatus(operations.cancel(requireOpenHandle()), operations);
    }

    public GenericUsbInterface claimInterface(int interfaceNumber) throws UsbTransportException {
        requireWorkerThread(isMainThread());
        requireUnsigned(interfaceNumber, 0xff, "interfaceNumber");
        long handle = requireOpenHandle();
        checkStatus(operations.claimInterface(handle, interfaceNumber), operations);
        try {
            refreshDescriptor(handle);
            GenericUsbInterfaceDescriptor interfaceDescriptor = findInterface(interfaceNumber);
            if (interfaceDescriptor == null || !interfaceDescriptor.isClaimed()) {
                throw failure(UsbTransportStatus.INTERNAL_ERROR,
                        "Claimed interface is absent from the native snapshot");
            }
            return new GenericUsbInterface(this, interfaceDescriptor);
        } catch (UsbTransportException | RuntimeException error) {
            operations.releaseInterface(handle, interfaceNumber);
            throw error;
        }
    }

    public boolean isOpen() {
        synchronized (stateLock) { return session != 0; }
    }

    @Override
    public void close() throws UsbTransportException {
        requireWorkerThread(isMainThread());
        long handle;
        synchronized (stateLock) {
            handle = session;
            session = 0;
        }
        if (handle == 0) return;
        checkStatus(operations.close(handle), operations);
    }

    long requireOpenHandle() throws UsbTransportException {
        synchronized (stateLock) {
            if (session == 0) throw failure(UsbTransportStatus.INVALID_STATE, "Session is closed");
            return session;
        }
    }

    TransportOperations operations() { return operations; }

    Object ownerToken() { return ownerToken; }

    GenericUsbInterfaceDescriptor findInterface(int interfaceNumber) {
        GenericUsbConfiguration active = getActiveConfiguration();
        if (active == null) return null;
        for (GenericUsbInterfaceDescriptor value : active.getInterfaces()) {
            if (value.getInterfaceNumber() == interfaceNumber) return value;
        }
        return null;
    }

    void refreshDescriptor(long expectedHandle) throws UsbTransportException {
        GenericUsbDeviceDescriptor refreshed = readDescriptor(
                expectedHandle, operations, ownerToken);
        synchronized (stateLock) {
            if (session != expectedHandle) throw failure(
                    UsbTransportStatus.INVALID_STATE, "Session closed during operation");
            descriptor = refreshed;
        }
    }

    private static GenericUsbDeviceDescriptor readDescriptor(
            long session, TransportOperations operations, Object ownerToken)
            throws UsbTransportException {
        long[] device = operations.getDeviceDescriptor(session);
        requireRecord(device, TransportNativeBridge.DEVICE_RECORD_LENGTH, operations);
        checkStatus((int) device[0], operations);
        long generation = device[1];
        int configurationCount = count(device[10], "configuration count");
        List<GenericUsbConfiguration> configurations = new ArrayList<>(configurationCount);
        for (int configurationIndex = 0; configurationIndex < configurationCount;
                ++configurationIndex) {
            configurations.add(readConfiguration(
                    session, operations, generation, configurationIndex, ownerToken));
        }
        try {
            return new GenericUsbDeviceDescriptor((int) device[2], (int) device[3],
                    (int) device[4], (int) device[5], (int) device[6], (int) device[7],
                    (int) device[8], (int) device[9], configurations, generation);
        } catch (IllegalArgumentException error) {
            throw failure(UsbTransportStatus.INTERNAL_ERROR, "Invalid native device descriptor");
        }
    }

    private static GenericUsbConfiguration readConfiguration(long session,
            TransportOperations operations, long generation, int configurationIndex,
            Object ownerToken)
            throws UsbTransportException {
        long[] value = operations.getConfiguration(session, configurationIndex);
        requireRecord(value, TransportNativeBridge.CONFIGURATION_RECORD_LENGTH, operations);
        checkStatus((int) value[0], operations);
        requireGeneration(generation, value[1]);
        int interfaceCount = count(value[7], "interface count");
        List<GenericUsbInterfaceDescriptor> interfaces = new ArrayList<>(interfaceCount);
        for (int interfaceIndex = 0; interfaceIndex < interfaceCount; ++interfaceIndex) {
            interfaces.add(readInterface(session, operations, generation,
                    configurationIndex, interfaceIndex, ownerToken));
        }
        List<AdditionalUsbDescriptor> additional = readAdditional(session, operations, 1,
                generation, configurationIndex, 0, 0, 0, count(value[8], "descriptor count"));
        return new GenericUsbConfiguration((int) value[3], (int) value[4], (int) value[5],
                value[6] != 0, interfaces, additional, generation);
    }

    private static GenericUsbInterfaceDescriptor readInterface(long session,
            TransportOperations operations, long generation, int configurationIndex,
            int interfaceIndex, Object ownerToken) throws UsbTransportException {
        long[] value = operations.getInterface(session, configurationIndex, interfaceIndex);
        requireRecord(value, TransportNativeBridge.INTERFACE_RECORD_LENGTH, operations);
        checkStatus((int) value[0], operations);
        requireGeneration(generation, value[1]);
        int alternateCount = count(value[6], "alternate-setting count");
        List<GenericUsbAlternateSetting> alternates = new ArrayList<>(alternateCount);
        for (int alternateIndex = 0; alternateIndex < alternateCount; ++alternateIndex) {
            alternates.add(readAlternate(session, operations, generation,
                    configurationIndex, interfaceIndex, alternateIndex, ownerToken));
        }
        return new GenericUsbInterfaceDescriptor((int) value[3], (int) value[4], value[5] != 0,
                alternates, generation);
    }

    private static GenericUsbAlternateSetting readAlternate(long session,
            TransportOperations operations, long generation, int configurationIndex,
            int interfaceIndex, int alternateIndex, Object ownerToken)
            throws UsbTransportException {
        long[] value = operations.getAlternateSetting(
                session, configurationIndex, interfaceIndex, alternateIndex);
        requireRecord(value, TransportNativeBridge.ALTERNATE_RECORD_LENGTH, operations);
        checkStatus((int) value[0], operations);
        requireGeneration(generation, value[1]);
        int endpointCount = count(value[8], "endpoint count");
        List<GenericUsbEndpoint> endpoints = new ArrayList<>(endpointCount);
        for (int endpointIndex = 0; endpointIndex < endpointCount; ++endpointIndex) {
            endpoints.add(readEndpoint(session, operations, generation,
                    configurationIndex, interfaceIndex, alternateIndex, endpointIndex, ownerToken));
        }
        List<AdditionalUsbDescriptor> additional = readAdditional(session, operations, 2,
                generation, configurationIndex, interfaceIndex, alternateIndex, 0,
                count(value[9], "descriptor count"));
        return new GenericUsbAlternateSetting((int) value[3], (int) value[4], (int) value[5],
                (int) value[6], (int) value[7], endpoints, additional, generation);
    }

    private static GenericUsbEndpoint readEndpoint(long session, TransportOperations operations,
            long generation, int configurationIndex, int interfaceIndex, int alternateIndex,
            int endpointIndex, Object ownerToken) throws UsbTransportException {
        long[] value = operations.getEndpoint(
                session, configurationIndex, interfaceIndex, alternateIndex, endpointIndex);
        requireRecord(value, TransportNativeBridge.ENDPOINT_RECORD_LENGTH, operations);
        checkStatus((int) value[0], operations);
        requireGeneration(generation, value[1]);
        List<AdditionalUsbDescriptor> additional = readAdditional(session, operations, 3,
                generation, configurationIndex, interfaceIndex, alternateIndex, endpointIndex,
                count(value[9], "descriptor count"));
        UsbDirection direction = value[5] == 0 ? UsbDirection.OUT : UsbDirection.IN;
        UsbTransferType type;
        switch ((int) value[6]) {
            case 0: type = UsbTransferType.CONTROL; break;
            case 1: type = UsbTransferType.ISOCHRONOUS; break;
            case 2: type = UsbTransferType.BULK; break;
            case 3: type = UsbTransferType.INTERRUPT; break;
            default: throw failure(UsbTransportStatus.INTERNAL_ERROR,
                    "Invalid native endpoint transfer type");
        }
        return new GenericUsbEndpoint((int) value[3], (int) value[4], direction, type,
                (int) value[7], (int) value[8], additional, generation, ownerToken);
    }

    private static List<AdditionalUsbDescriptor> readAdditional(long session,
            TransportOperations operations, int scope, long generation, int configurationIndex,
            int interfaceIndex, int alternateIndex, int endpointIndex, int descriptorCount)
            throws UsbTransportException {
        if (descriptorCount == 0) return Collections.emptyList();
        List<AdditionalUsbDescriptor> result = new ArrayList<>(descriptorCount);
        for (int index = 0; index < descriptorCount; ++index) {
            byte[] payload = operations.getAdditionalDescriptor(session, scope, generation,
                    configurationIndex, interfaceIndex, alternateIndex, endpointIndex, index);
            if (payload == null || payload.length == 0) throw failure(
                    UsbTransportStatus.INTERNAL_ERROR, "Invalid native descriptor payload");
            checkStatus(payload[0] & 0xff, operations);
            if (payload.length < 2) throw failure(
                    UsbTransportStatus.INTERNAL_ERROR, "Truncated native descriptor payload");
            byte[] raw = new byte[payload.length - 2];
            System.arraycopy(payload, 2, raw, 0, raw.length);
            result.add(new AdditionalUsbDescriptor(payload[1] & 0xff, raw));
        }
        return result;
    }

    static void requireWorkerThread(boolean mainThread) throws UsbTransportException {
        if (mainThread) throw failure(UsbTransportStatus.INVALID_STATE,
                "Blocking USB operations are not allowed on the Android main thread");
    }

    static boolean isMainThread() {
        Looper main = Looper.getMainLooper();
        return main != null && Looper.myLooper() == main;
    }

    static void checkStatus(int code, TransportOperations operations)
            throws UsbTransportException {
        UsbTransportStatus status = UsbTransportStatus.fromCode(code);
        if (status != UsbTransportStatus.OK) throw new UsbTransportException(
                status, operations.lastError());
    }

    private static void requireRecord(long[] record, int expected, TransportOperations operations)
            throws UsbTransportException {
        if (record == null || record.length == 0) throw failure(
                UsbTransportStatus.INTERNAL_ERROR, "Native transport returned no result");
        checkStatus((int) record[0], operations);
        if (record.length != expected) throw failure(
                UsbTransportStatus.INTERNAL_ERROR, "Native transport returned an invalid record");
    }

    private static void requireGeneration(long expected, long actual)
            throws UsbTransportException {
        if (actual != expected) throw failure(
                UsbTransportStatus.INVALID_STATE, "Descriptor snapshot changed during enumeration");
    }

    private static int count(long value, String name) throws UsbTransportException {
        if (value < 0 || value > Integer.MAX_VALUE) throw failure(
                UsbTransportStatus.INTERNAL_ERROR, "Invalid native " + name);
        return (int) value;
    }

    private static void requireUnsigned(int value, int maximum, String name)
            throws UsbTransportException {
        if (value < 0 || value > maximum) throw failure(
                UsbTransportStatus.INVALID_ARGUMENT, name + " is outside its unsigned range");
    }

    static UsbTransportException failure(UsbTransportStatus status, String message) {
        return new UsbTransportException(status, message);
    }
}

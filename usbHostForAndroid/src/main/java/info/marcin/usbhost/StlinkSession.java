package info.marcin.usbhost;

import android.hardware.usb.UsbDeviceConnection;
import android.os.Looper;

import java.io.Closeable;
import java.util.Objects;

/** Exclusive, serialized ST-Link session. */
public final class StlinkSession implements Closeable {
    public static final int MAX_READ_SIZE = 1024 * 1024;

    private final UsbDeviceConnection connection;
    private final ProgrammerInfo programmerInfo;
    private long nativeHandle;
    private TargetInfo targetInfo;
    private boolean closed;

    StlinkSession(UsbDeviceConnection connection, long nativeHandle, ProgrammerInfo programmerInfo) {
        this.connection = Objects.requireNonNull(connection, "connection");
        this.nativeHandle = nativeHandle;
        this.programmerInfo = Objects.requireNonNull(programmerInfo, "programmerInfo");
    }

    public synchronized ProgrammerInfo getProgrammerInfo() {
        return programmerInfo;
    }

    public synchronized TargetInfo getTargetInfo() {
        return targetInfo;
    }

    public synchronized boolean isOpen() {
        return !closed && nativeHandle != 0;
    }

    public synchronized TargetInfo connectTarget() throws StlinkException {
        requireWorkerThread();
        requireOpen();
        if (targetInfo != null) {
            return targetInfo;
        }
        long[] connected = NativeBridge.connectTarget(nativeHandle);
        if (connected == null || connected.length != 8) {
            throw new StlinkException(StlinkStatus.INTERNAL_ERROR,
                    "Native target connection returned an invalid result");
        }
        StlinkStatus status = StlinkStatus.fromCode((int) connected[0]);
        if (status != StlinkStatus.OK) {
            throw nativeFailure(status);
        }
        targetInfo = new TargetInfo(connected[1], connected[2], connected[3], connected[4],
                connected[5], connected[6], (int) connected[7]);
        return targetInfo;
    }

    public synchronized byte[] readMemory(long address, int length) throws StlinkException {
        requireWorkerThread();
        requireOpen();
        validateMemoryRequest(address, length);
        if (targetInfo == null) {
            throw new StlinkException(StlinkStatus.INVALID_STATE,
                    "Connect and identify the target before reading memory");
        }
        byte[] result = NativeBridge.readMemory(nativeHandle, address, length);
        if (result == null) {
            throw nativeFailure(StlinkStatus.fromCode(NativeBridge.lastStatus()));
        }
        return result;
    }

    @Override
    public synchronized void close() {
        if (closed) {
            return;
        }
        if (nativeHandle != 0) {
            NativeBridge.close(nativeHandle);
            nativeHandle = 0;
        }
        connection.close();
        closed = true;
    }

    static void validateMemoryRequest(long address, int length) throws StlinkException {
        if (address < 0 || address > 0xffffffffL || length <= 0 || length > MAX_READ_SIZE
                || address + (long) length > 0x1_0000_0000L) {
            throw new StlinkException(StlinkStatus.INVALID_ARGUMENT,
                    "Memory range must fit in 32-bit address space and be at most one MiB");
        }
    }

    static void requireWorkerThread() throws StlinkException {
        Looper main = Looper.getMainLooper();
        if (main != null && Looper.myLooper() == main) {
            throw new StlinkException(StlinkStatus.INVALID_STATE,
                    "Blocking ST-Link operations are not allowed on the Android main thread");
        }
    }

    private void requireOpen() throws StlinkException {
        if (nativeHandle == 0) {
            throw new StlinkException(StlinkStatus.INVALID_STATE, "Session is closed");
        }
    }

    private StlinkException nativeFailure(StlinkStatus status) {
        String message = NativeBridge.lastError();
        if (status == StlinkStatus.DISCONNECTED) {
            close();
        }
        return new StlinkException(status, message);
    }
}

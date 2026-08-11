package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

public class GenericUsbDeviceTest {
    @Test
    public void opensSnapshotWithoutTakingCallerConnectionOwnership() throws Exception {
        FakeTransport transport = new FakeTransport();
        CallerConnection connection = new CallerConnection(91);
        GenericUsbDevice device = GenericUsbDevice.openForTesting(
                connection, 0x1234, 0x5678, transport, false);

        assertEquals(91, transport.openedFileDescriptor);
        assertEquals(0, connection.closeCount);
        assertEquals(0x1234, device.getDescriptor().getVendorId());
        assertEquals(1, device.getConfigurations().size());
        assertEquals(1, device.getActiveConfiguration().getConfigurationValue());
        assertTrue(device.isOpen());

        device.close();
        device.close();
        assertEquals(1, transport.closeCount);
        assertEquals(0, connection.closeCount);
        assertFalse(device.isOpen());
    }

    @Test
    public void rejectsMainThreadAndMismatchedDevice() throws Exception {
        FakeTransport transport = new FakeTransport();
        assertStatus(UsbTransportStatus.INVALID_STATE, () -> GenericUsbDevice.openForTesting(
                new CallerConnection(92), 0x1234, 0x5678, transport, true));
        assertEquals(0, transport.openCount);

        assertStatus(UsbTransportStatus.UNSUPPORTED_DEVICE,
                () -> GenericUsbDevice.openForTesting(
                        new CallerConnection(93), 0xabcd, 0x5678, transport, false));
        assertEquals(1, transport.closeCount);
    }

    @Test
    public void refreshesAfterConfigurationSelectionAndExposesCancellation() throws Exception {
        FakeTransport transport = new FakeTransport();
        GenericUsbDevice device = GenericUsbDevice.openForTesting(
                new CallerConnection(94), 0x1234, 0x5678, transport, false);
        device.selectConfiguration(1);
        device.cancelActiveTransfer();
        assertEquals(1, transport.selectCount);
        assertEquals(1, transport.cancelCount);
        device.close();
    }

    private static void assertStatus(UsbTransportStatus expected, ThrowingCall call) {
        try {
            call.run();
            fail("Expected UsbTransportException");
        } catch (UsbTransportException error) {
            assertEquals(expected, error.getStatus());
        }
    }

    private interface ThrowingCall { void run() throws UsbTransportException; }

    private static final class CallerConnection implements GenericUsbDevice.FileDescriptorSource {
        final int fd;
        int closeCount;
        CallerConnection(int fd) { this.fd = fd; }
        @Override public int getFileDescriptor() { return fd; }
    }

    private static final class FakeTransport implements TransportOperations {
        int openedFileDescriptor = -1;
        int openCount;
        int closeCount;
        int selectCount;
        int cancelCount;

        @Override public long[] openSession(int fd) {
            openedFileDescriptor = fd;
            ++openCount;
            return new long[] {0, 41};
        }
        @Override public long[] getDeviceDescriptor(long session) {
            return new long[] {0, 1, 0x0200, 0, 0, 0, 64, 0x1234, 0x5678, 0x0100, 1};
        }
        @Override public long[] getConfiguration(long session, int configurationIndex) {
            return new long[] {0, 1, 0, 1, 0x80, 50, 1, 1, 0};
        }
        @Override public long[] getInterface(long session, int configurationIndex,
                int interfaceIndex) {
            return new long[] {0, 1, 0, 3, 0, 0, 1};
        }
        @Override public long[] getAlternateSetting(long session, int configurationIndex,
                int interfaceIndex, int alternateSettingIndex) {
            return new long[] {0, 1, 0, 3, 0, 0xff, 0, 0, 1, 0};
        }
        @Override public long[] getEndpoint(long session, int configurationIndex,
                int interfaceIndex, int alternateSettingIndex, int endpointIndex) {
            return new long[] {0, 1, 0, 0x81, 1, 1, 2, 64, 0, 0};
        }
        @Override public byte[] getAdditionalDescriptor(long session, int scope, long generation,
                int configurationIndex, int interfaceIndex, int alternateSettingIndex,
                int endpointIndex, int additionalDescriptorIndex) {
            throw new AssertionError("No fixture descriptors");
        }
        @Override public int selectConfiguration(long session, int value) {
            ++selectCount;
            return 0;
        }
        @Override public int claimInterface(long session, int value) { return 0; }
        @Override public int selectAlternateSetting(long session, int number, int alternate) {
            return 0;
        }
        @Override public int releaseInterface(long session, int value) { return 0; }
        @Override public int cancel(long session) { ++cancelCount; return 0; }
        @Override public int close(long session) { ++closeCount; return 0; }
        @Override public String lastError() { return "fake failure"; }
    }
}

package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

public class GenericUsbInterfaceTest {
    @Test
    public void rejectsForeignAndStaleEndpointsAndReleasesOnce() throws Exception {
        FakeTransport firstTransport = new FakeTransport(51);
        GenericUsbDevice first = open(firstTransport);
        GenericUsbInterface claimed = first.claimInterface(3);
        GenericUsbEndpoint original = claimed.getActiveAlternateSetting().getEndpoints().get(0);
        claimed.validateEndpoint(original);

        FakeTransport secondTransport = new FakeTransport(52);
        GenericUsbDevice second = open(secondTransport);
        GenericUsbEndpoint foreign = second.getActiveConfiguration().getInterfaces().get(0)
                .getAlternateSettings().get(0).getEndpoints().get(0);
        assertInvalid(() -> claimed.validateEndpoint(foreign));

        claimed.selectAlternateSetting(1);
        assertEquals(1, claimed.getActiveAlternateSetting().getAlternateSetting());
        assertInvalid(() -> claimed.validateEndpoint(original));
        assertTrue(claimed.isClaimed());

        claimed.close();
        claimed.close();
        assertFalse(claimed.isClaimed());
        assertEquals(1, firstTransport.releaseCount);
        first.close();
        second.close();
    }

    private static GenericUsbDevice open(FakeTransport transport) throws UsbTransportException {
        return GenericUsbDevice.openForTesting(() -> 90, 0x1234, 0x5678, transport, false);
    }

    private static void assertInvalid(ThrowingCall call) {
        try {
            call.run();
            fail("Expected stale or foreign endpoint rejection");
        } catch (UsbTransportException error) {
            assertEquals(UsbTransportStatus.INVALID_STATE, error.getStatus());
        }
    }

    private interface ThrowingCall { void run() throws UsbTransportException; }

    private static final class FakeTransport implements TransportOperations {
        final long session;
        long generation = 1;
        int activeAlternate;
        int releaseCount;
        FakeTransport(long session) { this.session = session; }
        @Override public long[] openSession(int fd) { return new long[] {0, session}; }
        @Override public long[] getDeviceDescriptor(long ignored) {
            return new long[] {0, generation, 0x200, 0, 0, 0, 64,
                    0x1234, 0x5678, 0x100, 1};
        }
        @Override public long[] getConfiguration(long ignored, int index) {
            return new long[] {0, generation, 0, 1, 0x80, 50, 1, 1, 0};
        }
        @Override public long[] getInterface(long ignored, int configuration, int index) {
            return new long[] {0, generation, 0, 3, activeAlternate, 1, 2};
        }
        @Override public long[] getAlternateSetting(
                long ignored, int configuration, int iface, int index) {
            return new long[] {0, generation, index, 3, index, 0xff, 0, 0, 1, 0};
        }
        @Override public long[] getEndpoint(
                long ignored, int configuration, int iface, int alternate, int index) {
            return new long[] {0, generation, 0, 0x81 + alternate, 1 + alternate,
                    1, 2, 64, 0, 0};
        }
        @Override public byte[] getAdditionalDescriptor(long ignored, int scope, long snapshot,
                int configuration, int iface, int alternate, int endpoint, int index) {
            throw new AssertionError("No fixture descriptors");
        }
        @Override public int selectConfiguration(long ignored, int value) { return 0; }
        @Override public int claimInterface(long ignored, int number) { return 0; }
        @Override public int selectAlternateSetting(long ignored, int number, int alternate) {
            activeAlternate = alternate;
            ++generation;
            return 0;
        }
        @Override public int releaseInterface(long ignored, int number) {
            ++releaseCount;
            return 0;
        }
        @Override public int cancel(long ignored) { return 0; }
        @Override public int close(long ignored) { return 0; }
        @Override public String lastError() { return "fake failure"; }
    }
}

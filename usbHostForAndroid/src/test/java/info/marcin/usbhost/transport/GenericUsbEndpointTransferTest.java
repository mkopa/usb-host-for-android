package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import org.junit.Test;

public class GenericUsbEndpointTransferTest {
    @Test
    public void performsBulkAndInterruptAndPreservesPartialFailure() throws Exception {
        GenericUsbInterfaceTest.FakeTransport transport =
                new GenericUsbInterfaceTest.FakeTransport(61);
        GenericUsbDevice device = open(transport, 97);
        GenericUsbInterface claimed = device.claimInterface(3);
        GenericUsbEndpoint bulk = claimed.getActiveAlternateSetting().getEndpoints().get(0);
        byte[] buffer = new byte[8];

        transport.bulkResult = new long[] {UsbTransportStatus.OK.getCode(), 3};
        assertEquals(3, claimed.bulkTransferForTesting(
                bulk, buffer, 2, 4, 50, false).getActualLength());
        assertEquals(0x81, transport.endpointAddress);

        transport.bulkResult = new long[] {UsbTransportStatus.STALL.getCode(), 2};
        try {
            claimed.bulkTransferForTesting(bulk, buffer, 2, 4, 50, false);
            fail("Expected partial stall");
        } catch (UsbTransportException error) {
            assertEquals(UsbTransportStatus.STALL, error.getStatus());
            assertEquals(2, error.getActualLength());
        }

        claimed.selectAlternateSetting(1);
        GenericUsbEndpoint interrupt = claimed.getActiveAlternateSetting().getEndpoints().get(0);
        transport.interruptResult = new long[] {UsbTransportStatus.OK.getCode(), 1};
        assertEquals(1, claimed.interruptTransferForTesting(
                interrupt, buffer, 0, 4, 50, false).getActualLength());
        assertEquals(0x82, transport.endpointAddress);
        claimed.close();
        device.close();
    }

    @Test
    public void rejectsMainThreadWrongTypeAndForeignEndpointBeforeNativeCall() throws Exception {
        GenericUsbInterfaceTest.FakeTransport firstTransport =
                new GenericUsbInterfaceTest.FakeTransport(62);
        GenericUsbDevice first = open(firstTransport, 98);
        GenericUsbInterface claimed = first.claimInterface(3);
        GenericUsbEndpoint bulk = claimed.getActiveAlternateSetting().getEndpoints().get(0);

        assertStatus(UsbTransportStatus.INVALID_STATE, () -> claimed.bulkTransferForTesting(
                bulk, new byte[4], 0, 4, 50, true));
        assertStatus(UsbTransportStatus.INVALID_ARGUMENT,
                () -> claimed.interruptTransferForTesting(
                        bulk, new byte[4], 0, 4, 50, false));

        GenericUsbInterfaceTest.FakeTransport secondTransport =
                new GenericUsbInterfaceTest.FakeTransport(63);
        GenericUsbDevice second = open(secondTransport, 99);
        GenericUsbEndpoint foreign = second.getActiveConfiguration().getInterfaces().get(0)
                .getAlternateSettings().get(0).getEndpoints().get(0);
        assertStatus(UsbTransportStatus.INVALID_STATE, () -> claimed.bulkTransferForTesting(
                foreign, new byte[4], 0, 4, 50, false));
        assertEquals(0, firstTransport.bulkCount);
        first.close();
        second.close();
    }

    private static GenericUsbDevice open(
            GenericUsbInterfaceTest.FakeTransport transport, int fd)
            throws UsbTransportException {
        return GenericUsbDevice.openForTesting(
                () -> fd, 0x1234, 0x5678, transport, false);
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
}

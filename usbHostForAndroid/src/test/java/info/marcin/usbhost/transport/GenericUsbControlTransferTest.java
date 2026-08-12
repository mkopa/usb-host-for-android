package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import org.junit.Test;

public class GenericUsbControlTransferTest {
    @Test
    public void returnsActualCountAndPreservesPartialFailure() throws Exception {
        GenericUsbDeviceTest.FakeTransport transport = new GenericUsbDeviceTest.FakeTransport();
        GenericUsbDevice device = GenericUsbDevice.openForTesting(
                () -> 95, 0x1234, 0x5678, transport, false);
        UsbControlRequest request = new UsbControlRequest(
                0x80, 6, 0x0100, 0, UsbDirection.IN);
        byte[] buffer = new byte[8];

        transport.controlResult = new long[] {UsbTransportStatus.OK.getCode(), 3};
        UsbTransferResult result = device.controlTransferForTesting(
                request, buffer, 2, 4, 50, false);
        assertEquals(UsbTransportStatus.OK, result.getStatus());
        assertEquals(3, result.getActualLength());
        assertEquals(2, transport.controlOffset);
        assertEquals(4, transport.controlLength);

        transport.controlResult = new long[] {UsbTransportStatus.TIMEOUT.getCode(), 2};
        try {
            device.controlTransferForTesting(request, buffer, 2, 4, 50, false);
            fail("Expected partial timeout");
        } catch (UsbTransportException error) {
            assertEquals(UsbTransportStatus.TIMEOUT, error.getStatus());
            assertEquals(2, error.getActualLength());
        }
        device.close();
    }

    @Test
    public void rejectsMainThreadAndInvalidSliceBeforeNativeCall() throws Exception {
        GenericUsbDeviceTest.FakeTransport transport = new GenericUsbDeviceTest.FakeTransport();
        GenericUsbDevice device = GenericUsbDevice.openForTesting(
                () -> 96, 0x1234, 0x5678, transport, false);
        UsbControlRequest request = new UsbControlRequest(
                0x00, 9, 0, 0, UsbDirection.OUT);

        assertStatus(UsbTransportStatus.INVALID_STATE, () -> device.controlTransferForTesting(
                request, new byte[8], 0, 4, 50, true));
        assertStatus(UsbTransportStatus.INVALID_ARGUMENT, () -> device.controlTransferForTesting(
                request, new byte[8], 7, 2, 50, false));
        assertStatus(UsbTransportStatus.INVALID_ARGUMENT, () -> device.controlTransferForTesting(
                request, new byte[8], 0, 4, 0, false));
        assertEquals(0, transport.controlCount);

        assertStatus(UsbTransportStatus.INVALID_STATE,
                () -> device.cancelActiveTransferForTesting(true));
        assertEquals(0, transport.cancelCount);
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
}

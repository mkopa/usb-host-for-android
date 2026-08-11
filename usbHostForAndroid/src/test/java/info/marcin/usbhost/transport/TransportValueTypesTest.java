package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

public final class TransportValueTypesTest {
    @Test
    public void directionAndTransferTypeExposeStableConstants() {
        assertEquals(UsbDirection.IN, UsbDirection.valueOf("IN"));
        assertEquals(UsbDirection.OUT, UsbDirection.valueOf("OUT"));
        assertEquals(4, UsbTransferType.values().length);
        assertEquals(UsbTransferType.CONTROL, UsbTransferType.valueOf("CONTROL"));
        assertEquals(UsbTransferType.BULK, UsbTransferType.valueOf("BULK"));
        assertEquals(UsbTransferType.INTERRUPT, UsbTransferType.valueOf("INTERRUPT"));
        assertEquals(UsbTransferType.ISOCHRONOUS, UsbTransferType.valueOf("ISOCHRONOUS"));
    }

    @Test
    public void controlRequestValidatesUnsignedFieldsAndDirection() {
        UsbControlRequest minimum = new UsbControlRequest(0, 0, 0, 0, UsbDirection.OUT);
        UsbControlRequest maximum = new UsbControlRequest(
                0xff, 0xff, 0xffff, 0xffff, UsbDirection.IN);
        assertEquals(0, minimum.getRequestType());
        assertEquals(0xff, maximum.getRequestType());
        assertEquals(0xff, maximum.getRequest());
        assertEquals(0xffff, maximum.getValue());
        assertEquals(0xffff, maximum.getIndex());
        assertEquals(UsbDirection.IN, maximum.getDirection());

        assertIllegalArgument(() -> new UsbControlRequest(-1, 0, 0, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0x100, 0, 0, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, -1, 0, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0x100, 0, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0, -1, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0, 0x10000, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0, 0, -1, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0, 0, 0x10000, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0x80, 0, 0, 0, UsbDirection.OUT));
        assertIllegalArgument(() -> new UsbControlRequest(0, 0, 0, 0, UsbDirection.IN));
        assertNullPointer(() -> new UsbControlRequest(0, 0, 0, 0, null));
    }

    @Test
    public void controlRequestHasValueEquality() {
        UsbControlRequest first = new UsbControlRequest(0x80, 6, 0x100, 2, UsbDirection.IN);
        UsbControlRequest equal = new UsbControlRequest(0x80, 6, 0x100, 2, UsbDirection.IN);
        UsbControlRequest different = new UsbControlRequest(0, 6, 0x100, 2, UsbDirection.OUT);
        assertEquals(first, equal);
        assertEquals(first.hashCode(), equal.hashCode());
        assertNotEquals(first, different);
        assertNotEquals(first, null);
    }

    @Test
    public void transferResultValidatesCountAndHasValueEquality() {
        UsbTransferResult shortPacket = new UsbTransferResult(UsbTransportStatus.OK, 17);
        assertEquals(UsbTransportStatus.OK, shortPacket.getStatus());
        assertEquals(17, shortPacket.getActualLength());
        assertTrue(shortPacket.isSuccessful());
        assertEquals(shortPacket, new UsbTransferResult(UsbTransportStatus.OK, 17));
        assertEquals(shortPacket.hashCode(), new UsbTransferResult(UsbTransportStatus.OK, 17).hashCode());

        UsbTransferResult timeout = new UsbTransferResult(UsbTransportStatus.TIMEOUT, 3);
        assertFalse(timeout.isSuccessful());
        assertNotEquals(shortPacket, timeout);
        assertIllegalArgument(() -> new UsbTransferResult(UsbTransportStatus.OK, -1));
        assertIllegalArgument(() -> new UsbTransferResult(UsbTransportStatus.OK, 1_048_577));
        assertNullPointer(() -> new UsbTransferResult(null, 0));
    }

    @Test
    public void transportExceptionCarriesStablePartialResultAndSanitizedMessage() {
        UsbTransportException exception = new UsbTransportException(
                UsbTransportStatus.CANCELLED, 23, "cancelled\u0000by\ncaller");
        assertEquals(UsbTransportStatus.CANCELLED, exception.getStatus());
        assertEquals(23, exception.getActualLength());
        assertEquals("cancelled?by?caller", exception.getMessage());

        UsbTransportException fallback = new UsbTransportException(
                UsbTransportStatus.TIMEOUT, 0, null);
        assertEquals("TIMEOUT", fallback.getMessage());
        assertIllegalArgument(() -> new UsbTransportException(UsbTransportStatus.OK, 0, "ok"));
        assertIllegalArgument(() -> new UsbTransportException(
                UsbTransportStatus.USB_ERROR, -1, "error"));
        assertIllegalArgument(() -> new UsbTransportException(
                UsbTransportStatus.USB_ERROR, 1_048_577, "error"));
        assertNullPointer(() -> new UsbTransportException(null, 0, "error"));
    }

    @Test
    public void transportExceptionBoundsDiagnostics() {
        StringBuilder input = new StringBuilder();
        for (int index = 0; index < 300; ++index) {
            input.append('x');
        }
        UsbTransportException exception = new UsbTransportException(
                UsbTransportStatus.INTERNAL_ERROR, input.toString());
        assertEquals(240, exception.getMessage().length());
        assertEquals(0, exception.getActualLength());
    }

    private static void assertIllegalArgument(ThrowingRunnable runnable) {
        try {
            runnable.run();
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException expected) {
            // Expected.
        } catch (Exception unexpected) {
            throw new AssertionError(unexpected);
        }
    }

    private static void assertNullPointer(ThrowingRunnable runnable) {
        try {
            runnable.run();
            fail("Expected NullPointerException");
        } catch (NullPointerException expected) {
            // Expected.
        } catch (Exception unexpected) {
            throw new AssertionError(unexpected);
        }
    }

    private interface ThrowingRunnable {
        void run() throws Exception;
    }
}

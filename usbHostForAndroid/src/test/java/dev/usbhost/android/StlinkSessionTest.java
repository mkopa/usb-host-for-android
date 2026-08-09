package dev.usbhost.android;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import org.junit.Test;

public class StlinkSessionTest {
    @Test
    public void validatesReadBoundsBeforeNativeIo() throws Exception {
        StlinkSession.validateMemoryRequest(0x08000001L, 1);
        StlinkSession.validateMemoryRequest(0xffffffffL, 1);
        assertInvalid(-1, 1);
        assertInvalid(0, 0);
        assertInvalid(0xffffffffL, 2);
        assertInvalid(0, StlinkSession.MAX_READ_SIZE + 1);
    }

    private static void assertInvalid(long address, int length) {
        try {
            StlinkSession.validateMemoryRequest(address, length);
            fail("Expected StlinkException");
        } catch (StlinkException error) {
            assertEquals(StlinkStatus.INVALID_ARGUMENT, error.getStatus());
        }
    }
}

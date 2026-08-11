package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class UsbTransportStatusTest {
    @Test
    public void codesRemainIdenticalToNativeAbi() {
        UsbTransportStatus[] statuses = UsbTransportStatus.values();
        assertEquals(16, statuses.length);
        for (int code = 0; code < statuses.length; ++code) {
            assertEquals(code, statuses[code].getCode());
            assertEquals(statuses[code], UsbTransportStatus.fromCode(code));
        }
    }

    @Test
    public void unknownCodeMapsToInternalError() {
        assertEquals(UsbTransportStatus.INTERNAL_ERROR, UsbTransportStatus.fromCode(-1));
        assertEquals(UsbTransportStatus.INTERNAL_ERROR, UsbTransportStatus.fromCode(16));
    }
}

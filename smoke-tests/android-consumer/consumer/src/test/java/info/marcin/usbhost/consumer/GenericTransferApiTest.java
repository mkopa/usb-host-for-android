package info.marcin.usbhost.consumer;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import info.marcin.usbhost.transport.UsbTransferResult;
import info.marcin.usbhost.transport.UsbTransportStatus;

import org.junit.Test;

public class GenericTransferApiTest {
    @Test
    public void adapterResultDefensivelyOwnsDescriptorPayload() {
        byte[] payload = {1, 2, 3};
        GenericTransferConsumer.AdapterResult result =
                new GenericTransferConsumer.AdapterResult(0x24, payload,
                        new UsbTransferResult(UsbTransportStatus.OK, 2));
        payload[0] = 9;

        assertEquals(0x24, result.getDescriptorType());
        assertArrayEquals(new byte[] {1, 2, 3}, result.getDescriptorBytes());
        byte[] returned = result.getDescriptorBytes();
        returned[1] = 9;
        assertArrayEquals(new byte[] {1, 2, 3}, result.getDescriptorBytes());
        assertEquals(2, result.getTransferResult().getActualLength());
    }
}

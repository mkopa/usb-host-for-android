package info.marcin.usbhost.transport;

import static org.junit.Assert.assertEquals;

import java.util.List;

import org.junit.Test;

public class ProtocolAdapterContractTest {
    @Test
    public void adapterReadsAdditionalDescriptorAndRunsOnePublicTransfer() throws Exception {
        GenericUsbInterfaceTest.FakeTransport transport =
                new GenericUsbInterfaceTest.FakeTransport(71);
        transport.additionalDescriptor = new byte[] {0, 0x24, 3, 0x24, 0x55};
        transport.controlResult = new long[] {UsbTransportStatus.OK.getCode(), 2};
        GenericUsbDevice device = GenericUsbDevice.openForTesting(
                () -> 101, 0x1234, 0x5678, transport, false);

        PublicProtocolAdapter.Result result = PublicProtocolAdapter.execute(
                device, 3, new UsbControlRequest(
                        0x80, 6, 0x0100, 0, UsbDirection.IN), new byte[8]);

        assertEquals(0x24, result.descriptor.getType());
        assertEquals(2, result.transfer.getActualLength());
        assertEquals(1, transport.controlCount);
        device.close();
    }

    /** Fixture adapter: every referenced library type and method is public. */
    private static final class PublicProtocolAdapter {
        static Result execute(GenericUsbDevice device, int interfaceNumber,
                UsbControlRequest request, byte[] buffer) throws UsbTransportException {
            GenericUsbConfiguration active = device.getActiveConfiguration();
            AdditionalUsbDescriptor descriptor = firstAdditional(active);
            try (GenericUsbInterface ignored = device.claimInterface(interfaceNumber)) {
                return new Result(descriptor,
                        device.controlTransfer(request, buffer, 0, buffer.length, 100));
            }
        }

        private static AdditionalUsbDescriptor firstAdditional(
                GenericUsbConfiguration configuration) throws UsbTransportException {
            List<AdditionalUsbDescriptor> values = configuration.getAdditionalDescriptors();
            if (values.isEmpty()) throw new UsbTransportException(
                    UsbTransportStatus.INVALID_STATE, "Fixture descriptor is absent");
            return values.get(0);
        }

        private static final class Result {
            final AdditionalUsbDescriptor descriptor;
            final UsbTransferResult transfer;
            Result(AdditionalUsbDescriptor descriptor, UsbTransferResult transfer) {
                this.descriptor = descriptor;
                this.transfer = transfer;
            }
        }
    }
}

package info.marcin.usbhost.transport;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.junit.Test;

public final class DescriptorModelTest {
    @Test
    public void additionalDescriptorDefensivelyCopiesBytes() {
        byte[] raw = {3, 0x24, (byte) 0xaa};
        AdditionalUsbDescriptor descriptor = new AdditionalUsbDescriptor(0x24, raw);
        raw[2] = 0;
        assertArrayEquals(new byte[] {3, 0x24, (byte) 0xaa}, descriptor.getBytes());
        byte[] returned = descriptor.getBytes();
        returned[2] = 1;
        assertEquals((byte) 0xaa, descriptor.getBytes()[2]);
        assertEquals(descriptor, new AdditionalUsbDescriptor(
                0x24, new byte[] {3, 0x24, (byte) 0xaa}));
        assertEquals(descriptor.hashCode(), new AdditionalUsbDescriptor(
                0x24, new byte[] {3, 0x24, (byte) 0xaa}).hashCode());
    }

    @Test
    public void nestedListsAreDefensiveAndUnmodifiable() {
        AdditionalUsbDescriptor extra = extra();
        List<AdditionalUsbDescriptor> extras = new ArrayList<>();
        extras.add(extra);
        GenericUsbEndpoint endpoint = endpoint(1, extras);
        extras.clear();
        assertEquals(1, endpoint.getAdditionalDescriptors().size());
        assertUnmodifiable(endpoint.getAdditionalDescriptors());

        List<GenericUsbEndpoint> endpoints = new ArrayList<>();
        endpoints.add(endpoint);
        GenericUsbAlternateSetting alternate = new GenericUsbAlternateSetting(
                3, 0, 0xff, 1, 2, endpoints, Collections.singletonList(extra), 1);
        endpoints.clear();
        assertEquals(1, alternate.getEndpoints().size());
        assertUnmodifiable(alternate.getEndpoints());

        GenericUsbInterfaceDescriptor interfaceDescriptor = new GenericUsbInterfaceDescriptor(
                3, 0, false, Collections.singletonList(alternate), 1);
        GenericUsbConfiguration configuration = new GenericUsbConfiguration(
                1, 0x80, 50, true, Collections.singletonList(interfaceDescriptor),
                Collections.singletonList(extra), 1);
        GenericUsbDeviceDescriptor device = new GenericUsbDeviceDescriptor(
                0x0210, 0xef, 2, 1, 64, 0x1234, 0x5678, 0x0100,
                Collections.singletonList(configuration), 1);
        assertUnmodifiable(interfaceDescriptor.getAlternateSettings());
        assertUnmodifiable(configuration.getInterfaces());
        assertUnmodifiable(configuration.getAdditionalDescriptors());
        assertUnmodifiable(device.getConfigurations());
        assertEquals(1, device.getConfigurationCount());
    }

    @Test
    public void allDescriptorValuesHaveDeepEquality() {
        GenericUsbDeviceDescriptor first = device(1);
        GenericUsbDeviceDescriptor equal = device(1);
        GenericUsbDeviceDescriptor newerGeneration = device(2);
        assertEquals(first, equal);
        assertEquals(first.hashCode(), equal.hashCode());
        assertNotEquals(first, newerGeneration);
        assertEquals(0x1234, first.getVendorId());
        assertEquals(0x81, first.getConfigurations().get(0).getInterfaces().get(0)
                .getAlternateSettings().get(0).getEndpoints().get(0).getAddress());
    }

    @Test
    public void invalidUnsignedFieldsAndRelationshipsAreRejected() {
        assertIllegal(() -> new AdditionalUsbDescriptor(256, new byte[] {2, 0}));
        assertIllegal(() -> new AdditionalUsbDescriptor(0x24, new byte[] {3, 0x24}));
        assertIllegal(() -> new AdditionalUsbDescriptor(0x24, new byte[] {2, 0x25}));
        assertIllegal(() -> new GenericUsbEndpoint(
                0x81, 2, UsbDirection.IN, UsbTransferType.BULK,
                64, 1, Collections.emptyList(), 1));
        assertIllegal(() -> new GenericUsbEndpoint(
                0x01, 1, UsbDirection.IN, UsbTransferType.BULK,
                64, 1, Collections.emptyList(), 1));
        assertIllegal(() -> new GenericUsbInterfaceDescriptor(
                256, 0, false, Collections.emptyList(), 1));
        assertIllegal(() -> new GenericUsbConfiguration(
                0, 0x80, 50, false, Collections.emptyList(), Collections.emptyList(), 1));
        assertIllegal(() -> new GenericUsbDeviceDescriptor(
                0x10000, 0, 0, 0, 64, 1, 2, 3, Collections.emptyList(), 1));
        assertIllegal(() -> endpoint(0, Collections.emptyList()));
    }

    private static GenericUsbDeviceDescriptor device(long generation) {
        GenericUsbEndpoint endpoint = endpoint(generation, Collections.singletonList(extra()));
        GenericUsbAlternateSetting alternate = new GenericUsbAlternateSetting(
                3, 0, 0xff, 1, 2, Collections.singletonList(endpoint),
                Collections.singletonList(extra()), generation);
        GenericUsbInterfaceDescriptor interfaceDescriptor = new GenericUsbInterfaceDescriptor(
                3, 0, false, Collections.singletonList(alternate), generation);
        GenericUsbConfiguration configuration = new GenericUsbConfiguration(
                1, 0x80, 50, true, Collections.singletonList(interfaceDescriptor),
                Collections.singletonList(extra()), generation);
        return new GenericUsbDeviceDescriptor(
                0x0210, 0xef, 2, 1, 64, 0x1234, 0x5678, 0x0100,
                Collections.singletonList(configuration), generation);
    }

    private static GenericUsbEndpoint endpoint(
            long generation, List<AdditionalUsbDescriptor> extras) {
        return new GenericUsbEndpoint(
                0x81, 1, UsbDirection.IN, UsbTransferType.BULK,
                64, 1, extras, generation);
    }

    private static AdditionalUsbDescriptor extra() {
        return new AdditionalUsbDescriptor(0x24, new byte[] {3, 0x24, (byte) 0xaa});
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    private static void assertUnmodifiable(List<?> list) {
        try {
            ((List) list).add(new Object());
            fail("Expected an unmodifiable list");
        } catch (UnsupportedOperationException expected) {
            // Expected.
        }
    }

    private static void assertIllegal(Runnable action) {
        try {
            action.run();
            fail("Expected IllegalArgumentException");
        } catch (IllegalArgumentException expected) {
            // Expected.
        }
    }
}

package info.marcin.usbhost.consumer;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;

import java.util.Objects;

import info.marcin.usbhost.transport.AdditionalUsbDescriptor;
import info.marcin.usbhost.transport.GenericUsbAlternateSetting;
import info.marcin.usbhost.transport.GenericUsbConfiguration;
import info.marcin.usbhost.transport.GenericUsbDevice;
import info.marcin.usbhost.transport.GenericUsbEndpoint;
import info.marcin.usbhost.transport.GenericUsbInterface;
import info.marcin.usbhost.transport.GenericUsbInterfaceDescriptor;
import info.marcin.usbhost.transport.UsbControlRequest;
import info.marcin.usbhost.transport.UsbTransferResult;
import info.marcin.usbhost.transport.UsbTransportException;
import info.marcin.usbhost.transport.UsbTransportStatus;

/** Minimal detached adapter using only the published generic transport API. */
public final class GenericTransferConsumer {
    private GenericTransferConsumer() {}

    public static AdapterResult execute(UsbDevice device,
            UsbDeviceConnection callerOwnedConnection, Script script,
            byte[] buffer, int offset, int length, int timeoutMillis)
            throws UsbTransportException {
        Objects.requireNonNull(script, "script");
        try (GenericUsbDevice session = GenericUsbDevice.open(device, callerOwnedConnection)) {
            session.selectConfiguration(script.getConfigurationValue());
            GenericUsbConfiguration configuration = session.getActiveConfiguration();
            if (configuration == null) throw new UsbTransportException(
                    UsbTransportStatus.INVALID_STATE, "No active USB configuration");
            AdditionalUsbDescriptor descriptor = findDescriptor(
                    configuration, script.getDescriptorType());
            try (GenericUsbInterface claimed =
                    session.claimInterface(script.getInterfaceNumber())) {
                UsbTransferResult transfer = session.controlTransfer(
                        script.getRequest(), buffer, offset, length, timeoutMillis);
                return new AdapterResult(
                        descriptor.getType(), descriptor.getBytes(), transfer);
            }
        }
    }

    private static AdditionalUsbDescriptor findDescriptor(
            GenericUsbConfiguration configuration, int expectedType)
            throws UsbTransportException {
        for (AdditionalUsbDescriptor descriptor : configuration.getAdditionalDescriptors())
            if (descriptor.getType() == expectedType) return descriptor;
        for (GenericUsbInterfaceDescriptor iface : configuration.getInterfaces()) {
            for (GenericUsbAlternateSetting alternate : iface.getAlternateSettings()) {
                for (AdditionalUsbDescriptor descriptor : alternate.getAdditionalDescriptors())
                    if (descriptor.getType() == expectedType) return descriptor;
                for (GenericUsbEndpoint endpoint : alternate.getEndpoints())
                    for (AdditionalUsbDescriptor descriptor : endpoint.getAdditionalDescriptors())
                        if (descriptor.getType() == expectedType) return descriptor;
            }
        }
        throw new UsbTransportException(
                UsbTransportStatus.INVALID_STATE, "Required additional descriptor is absent");
    }

    /** Immutable one-transfer protocol script. */
    public static final class Script {
        private final int configurationValue;
        private final int interfaceNumber;
        private final int descriptorType;
        private final UsbControlRequest request;

        public Script(int configurationValue, int interfaceNumber,
                int descriptorType, UsbControlRequest request) {
            if (configurationValue < 1 || configurationValue > 0xff
                    || interfaceNumber < 0 || interfaceNumber > 0xff
                    || descriptorType < 0 || descriptorType > 0xff) {
                throw new IllegalArgumentException("Script USB fields are outside their range");
            }
            this.configurationValue = configurationValue;
            this.interfaceNumber = interfaceNumber;
            this.descriptorType = descriptorType;
            this.request = Objects.requireNonNull(request, "request");
        }

        public int getConfigurationValue() { return configurationValue; }
        public int getInterfaceNumber() { return interfaceNumber; }
        public int getDescriptorType() { return descriptorType; }
        public UsbControlRequest getRequest() { return request; }
    }

    /** Immutable adapter outcome with a defensive descriptor payload copy. */
    public static final class AdapterResult {
        private final int descriptorType;
        private final byte[] descriptorBytes;
        private final UsbTransferResult transferResult;

        public AdapterResult(
                int descriptorType, byte[] descriptorBytes, UsbTransferResult transferResult) {
            if (descriptorType < 0 || descriptorType > 0xff)
                throw new IllegalArgumentException("descriptorType is outside its range");
            this.descriptorType = descriptorType;
            this.descriptorBytes = Objects.requireNonNull(
                    descriptorBytes, "descriptorBytes").clone();
            this.transferResult = Objects.requireNonNull(transferResult, "transferResult");
        }

        public int getDescriptorType() { return descriptorType; }
        public byte[] getDescriptorBytes() { return descriptorBytes.clone(); }
        public UsbTransferResult getTransferResult() { return transferResult; }
    }
}

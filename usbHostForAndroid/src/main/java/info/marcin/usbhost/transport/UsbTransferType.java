package info.marcin.usbhost.transport;

/** Standard USB transfer type. Isochronous endpoints are metadata-only in the initial API. */
public enum UsbTransferType {
    CONTROL,
    BULK,
    INTERRUPT,
    ISOCHRONOUS
}

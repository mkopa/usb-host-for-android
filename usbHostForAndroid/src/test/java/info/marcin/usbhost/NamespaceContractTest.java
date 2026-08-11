package info.marcin.usbhost;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public final class NamespaceContractTest {
    @Test
    public void publicTypesUseRequiredNamespace() {
        assertEquals("info.marcin.usbhost.StlinkSession", StlinkSession.class.getName());
        assertEquals("info.marcin.usbhost.StlinkDevice", StlinkDevice.class.getName());
        assertEquals("info.marcin.usbhost.TargetInfo", TargetInfo.class.getName());
        assertEquals("info.marcin.usbhost.NativeBridge", NativeBridge.class.getName());
    }
}

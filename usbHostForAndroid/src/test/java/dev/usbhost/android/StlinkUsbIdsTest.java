package dev.usbhost.android;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class StlinkUsbIdsTest {
    @Test
    public void acceptsOnlyDebugModeV3Products() {
        assertTrue(StlinkUsbIds.isSupported(0x0483, 0x374e));
        assertTrue(StlinkUsbIds.isSupported(0x0483, 0x374f));
        assertTrue(StlinkUsbIds.isSupported(0x0483, 0x3753));
        assertTrue(StlinkUsbIds.isSupported(0x0483, 0x3754));
        assertTrue(StlinkUsbIds.isSupported(0x0483, 0x3757));
        assertFalse(StlinkUsbIds.isSupported(0x0483, 0x374d));
        assertFalse(StlinkUsbIds.isSupported(0x0483, 0x3748));
        assertFalse(StlinkUsbIds.isSupported(0xffff, 0x374e));
    }
}

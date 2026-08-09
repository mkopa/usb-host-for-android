package dev.usbhost.android;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class TargetInfoTest {
    @Test
    public void exposesStm32G0b0Descriptor() {
        TargetInfo target = new TargetInfo(0x467, 0x08000000L, 512 * 1024L, 2048,
                0x20000000L, 144 * 1024L, 3300);
        assertEquals(0x467, target.getChipId());
        assertEquals("STM32G0Bx_G0Cx", target.getFamily());
        assertEquals(512 * 1024L, target.getFlashSize());
        assertEquals(144 * 1024L, target.getSramSize());
        assertEquals(3300, target.getTargetVoltageMv());
    }
}

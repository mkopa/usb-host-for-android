package info.marcin.usbhost;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

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

    @Test
    public void exposesOnlyTheReadOnlyProductSurface() {
        Set<String> publicMethods = new HashSet<>();
        for (Method method : StlinkSession.class.getDeclaredMethods()) {
            if (Modifier.isPublic(method.getModifiers()) && !method.isSynthetic()) {
                publicMethods.add(method.getName());
            }
        }
        assertEquals(new HashSet<>(Arrays.asList(
                "getProgrammerInfo", "getTargetInfo", "isOpen",
                "connectTarget", "readMemory", "close")), publicMethods);

        Set<String> nativeMethods = new HashSet<>();
        for (Method method : NativeBridge.class.getDeclaredMethods()) {
            if (Modifier.isNative(method.getModifiers())) {
                nativeMethods.add(method.getName());
            }
        }
        assertEquals(new HashSet<>(Arrays.asList(
                "open", "connectTarget", "readMemory", "close",
                "lastStatus", "lastError")), nativeMethods);

        for (String method : publicMethods) {
            String normalized = method.toLowerCase();
            assertFalse(normalized.contains("write"));
            assertFalse(normalized.contains("erase"));
            assertFalse(normalized.contains("reset"));
            assertFalse(normalized.contains("halt"));
            assertFalse(normalized.contains("step"));
            assertFalse(normalized.contains("run"));
        }
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

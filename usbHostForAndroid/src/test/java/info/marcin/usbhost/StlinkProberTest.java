package info.marcin.usbhost;

import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class StlinkProberTest {
    @Test
    public void identityOrderingIsDeterministic() {
        assertTrue(StlinkProber.compareIdentity(2, "z", 9, "a") < 0);
        assertTrue(StlinkProber.compareIdentity(2, "a", 2, "z") < 0);
        assertTrue(StlinkProber.compareIdentity(2, null, 2, "a") < 0);
    }
}

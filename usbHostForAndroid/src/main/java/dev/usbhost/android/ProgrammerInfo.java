package dev.usbhost.android;

import java.util.Objects;

/** Immutable ST-Link firmware information. */
public final class ProgrammerInfo {
    private final int stlinkVersion;
    private final int jtagVersion;
    private final int swimVersion;
    private final int jtagApiVersion;

    public ProgrammerInfo(int stlinkVersion, int jtagVersion, int swimVersion, int jtagApiVersion) {
        this.stlinkVersion = stlinkVersion;
        this.jtagVersion = jtagVersion;
        this.swimVersion = swimVersion;
        this.jtagApiVersion = jtagApiVersion;
    }

    public int getStlinkVersion() { return stlinkVersion; }
    public int getJtagVersion() { return jtagVersion; }
    public int getSwimVersion() { return swimVersion; }
    public int getJtagApiVersion() { return jtagApiVersion; }

    @Override
    public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof ProgrammerInfo)) return false;
        ProgrammerInfo that = (ProgrammerInfo) other;
        return stlinkVersion == that.stlinkVersion && jtagVersion == that.jtagVersion
                && swimVersion == that.swimVersion && jtagApiVersion == that.jtagApiVersion;
    }

    @Override
    public int hashCode() {
        return Objects.hash(stlinkVersion, jtagVersion, swimVersion, jtagApiVersion);
    }
}

package info.marcin.usbhost.transport;

import java.util.List;
import java.util.Objects;

/** Immutable group of alternate settings for one USB interface number. */
public final class GenericUsbInterfaceDescriptor {
    private final int interfaceNumber;
    private final int activeAlternateSetting;
    private final boolean claimed;
    private final List<GenericUsbAlternateSetting> alternateSettings;
    private final long snapshotGeneration;

    GenericUsbInterfaceDescriptor(int interfaceNumber, int activeAlternateSetting, boolean claimed,
            List<GenericUsbAlternateSetting> alternateSettings, long snapshotGeneration) {
        this.interfaceNumber = ManagedDescriptorSupport.unsigned(
                interfaceNumber, 0xff, "interfaceNumber");
        this.activeAlternateSetting = ManagedDescriptorSupport.unsigned(
                activeAlternateSetting, 0xff, "activeAlternateSetting");
        this.claimed = claimed;
        this.alternateSettings = ManagedDescriptorSupport.immutableList(
                alternateSettings, "alternateSettings");
        for (GenericUsbAlternateSetting alternate : this.alternateSettings) {
            if (alternate.getInterfaceNumber() != interfaceNumber
                    || alternate.getSnapshotGeneration() != snapshotGeneration) {
                throw new IllegalArgumentException("alternate setting parent mismatch");
            }
        }
        this.snapshotGeneration = ManagedDescriptorSupport.generation(snapshotGeneration);
    }

    public int getInterfaceNumber() { return interfaceNumber; }
    public int getActiveAlternateSetting() { return activeAlternateSetting; }
    public boolean isClaimed() { return claimed; }
    public List<GenericUsbAlternateSetting> getAlternateSettings() { return alternateSettings; }
    long getSnapshotGeneration() { return snapshotGeneration; }

    @Override public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof GenericUsbInterfaceDescriptor)) return false;
        GenericUsbInterfaceDescriptor that = (GenericUsbInterfaceDescriptor) other;
        return interfaceNumber == that.interfaceNumber
                && activeAlternateSetting == that.activeAlternateSetting && claimed == that.claimed
                && snapshotGeneration == that.snapshotGeneration
                && alternateSettings.equals(that.alternateSettings);
    }
    @Override public int hashCode() {
        return Objects.hash(interfaceNumber, activeAlternateSetting, claimed,
                alternateSettings, snapshotGeneration);
    }
}

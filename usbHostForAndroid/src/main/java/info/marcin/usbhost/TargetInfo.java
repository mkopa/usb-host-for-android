package info.marcin.usbhost;

import java.util.Objects;

/** Immutable information about a connected STM32 target. */
public final class TargetInfo {
    public static final long STM32G0BC_CHIP_ID = 0x467L;
    public static final String STM32G0BC_FAMILY = "STM32G0Bx_G0Cx";

    private final long chipId;
    private final long flashBase;
    private final long flashSize;
    private final long flashPageSize;
    private final long sramBase;
    private final long sramSize;
    private final int targetVoltageMv;

    public TargetInfo(long chipId, long flashBase, long flashSize, long flashPageSize,
                      long sramBase, long sramSize, int targetVoltageMv) {
        this.chipId = chipId;
        this.flashBase = flashBase;
        this.flashSize = flashSize;
        this.flashPageSize = flashPageSize;
        this.sramBase = sramBase;
        this.sramSize = sramSize;
        this.targetVoltageMv = targetVoltageMv;
    }

    public long getChipId() { return chipId; }
    public String getFamily() { return STM32G0BC_FAMILY; }
    public long getFlashBase() { return flashBase; }
    public long getFlashSize() { return flashSize; }
    public long getFlashPageSize() { return flashPageSize; }
    public long getSramBase() { return sramBase; }
    public long getSramSize() { return sramSize; }
    public int getTargetVoltageMv() { return targetVoltageMv; }

    @Override
    public boolean equals(Object other) {
        if (this == other) return true;
        if (!(other instanceof TargetInfo)) return false;
        TargetInfo that = (TargetInfo) other;
        return chipId == that.chipId && flashBase == that.flashBase
                && flashSize == that.flashSize && flashPageSize == that.flashPageSize
                && sramBase == that.sramBase && sramSize == that.sramSize
                && targetVoltageMv == that.targetVoltageMv;
    }

    @Override
    public int hashCode() {
        return Objects.hash(chipId, flashBase, flashSize, flashPageSize,
                sramBase, sramSize, targetVoltageMv);
    }
}

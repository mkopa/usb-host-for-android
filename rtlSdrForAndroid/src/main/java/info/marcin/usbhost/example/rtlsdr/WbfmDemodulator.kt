package info.marcin.usbhost.example.rtlsdr

/** Exact sample-rate and tuning profile used by rtl_fm's `-M wbfm` mode. */
object WbfmDemodulator {
    const val CAPTURE_RATE = 1_020_000
    const val AUDIO_RATE = 32_000
    const val FREQUENCY_HZ = 93_900_000L

    // rtl_fm's wideband controller adds 16 kHz before its quarter-rate offset tuning.
    const val CAPTURE_FREQUENCY_HZ = FREQUENCY_HZ + 16_000 + CAPTURE_RATE / 4
}

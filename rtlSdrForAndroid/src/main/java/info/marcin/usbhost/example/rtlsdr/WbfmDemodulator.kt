package info.marcin.usbhost.example.rtlsdr

import kotlin.math.abs

/** Real-time WBFM path matching rtl_fm's 170 kHz / fast atan / 32 kHz profile. */
class WbfmDemodulator {
    private var rotationPhase = 0
    private var iqCount = 0
    private var iAccumulator = 0
    private var qAccumulator = 0
    private var previousI = 0
    private var previousQ = 0
    private var havePrevious = false
    private var deemphasisAverage = 0
    private var resamplePhase = 0
    private var resampleAccumulator = 0
    private var dcAverage = 0

    fun process(input: ByteArray, length: Int): ShortArray {
        val maximumOutput = (length / 2 / INPUT_DOWNSAMPLE * AUDIO_RATE / IF_RATE) + 16
        val output = ShortArray(maximumOutput)
        var outputLength = 0
        var index = 0
        while (index + 1 < length) {
            var i = (input[index].toInt() and 0xff) - 127
            var q = (input[index + 1].toInt() and 0xff) - 127
            when (rotationPhase) {
                1 -> { val oldI = i; i = -q; q = oldI }
                2 -> { i = -i; q = -q }
                3 -> { val oldI = i; i = q; q = -oldI }
            }
            rotationPhase = (rotationPhase + 1) and 3
            iAccumulator += i
            qAccumulator += q
            iqCount++
            if (iqCount == INPUT_DOWNSAMPLE) {
                val filteredI = iAccumulator
                val filteredQ = qAccumulator
                iAccumulator = 0
                qAccumulator = 0
                iqCount = 0
                if (havePrevious) {
                    val real = filteredI * previousI + filteredQ * previousQ
                    val imaginary = filteredQ * previousI - filteredI * previousQ
                    var sample = fastAtan2(imaginary, real)

                    // rtl_fm deemphasis profile (75 us) at the 170 kHz discriminator rate.
                    val difference = sample - deemphasisAverage
                    deemphasisAverage += if (difference > 0) {
                        (difference + DEEMPHASIS_A / 2) / DEEMPHASIS_A
                    } else {
                        (difference - DEEMPHASIS_A / 2) / DEEMPHASIS_A
                    }
                    sample = deemphasisAverage

                    resampleAccumulator += sample
                    resamplePhase += AUDIO_RATE
                    if (resamplePhase >= IF_RATE) {
                        resamplePhase -= IF_RATE
                        var audio = resampleAccumulator / RESAMPLE_DIVISOR
                        resampleAccumulator = 0
                        // A slow DC estimator keeps the zero-IF residual out of the speaker.
                        dcAverage = (dcAverage * 999 + audio) / 1000
                        audio = (audio - dcAverage).coerceIn(Short.MIN_VALUE.toInt(), Short.MAX_VALUE.toInt())
                        if (outputLength < output.size) output[outputLength++] = audio.toShort()
                    }
                }
                previousI = filteredI
                previousQ = filteredQ
                havePrevious = true
            }
            index += 2
        }
        return output.copyOf(outputLength)
    }

    private fun fastAtan2(y: Int, x: Int): Int {
        if (x == 0 && y == 0) return 0
        val absoluteY = abs(y.toLong()).coerceAtMost(Int.MAX_VALUE.toLong()).toInt()
        val quarterPi = 1 shl 12
        val threeQuarterPi = 3 * quarterPi
        val angle = if (x >= 0) {
            val denominator = x.toLong() + absoluteY
            if (denominator == 0L) quarterPi
            else quarterPi - (quarterPi.toLong() * (x.toLong() - absoluteY) / denominator).toInt()
        } else {
            val denominator = absoluteY.toLong() - x
            if (denominator == 0L) threeQuarterPi
            else threeQuarterPi - (quarterPi.toLong() * (x.toLong() + absoluteY) / denominator).toInt()
        }
        return if (y < 0) -angle else angle
    }

    companion object {
        const val CAPTURE_RATE = 1_020_000
        const val IF_RATE = 170_000
        const val AUDIO_RATE = 32_000
        const val INPUT_DOWNSAMPLE = 6
        const val FREQUENCY_HZ = 93_900_000L
        const val CAPTURE_FREQUENCY_HZ = FREQUENCY_HZ + CAPTURE_RATE / 4
        private const val RESAMPLE_DIVISOR = IF_RATE / AUDIO_RATE
        private val DEEMPHASIS_A = kotlin.math.round(
            1.0 / (1.0 - kotlin.math.exp(-1.0 / (IF_RATE * 75e-6))),
        ).toInt()
    }
}

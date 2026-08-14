package info.marcin.usbhost.example.rtlsdr

object RtlSdrNative {
    init {
        System.loadLibrary("rtlsdr_audio")
    }

    fun open(authorizedFileDescriptor: Int, frequencyHz: Long, sampleRate: Int): Long {
        val handle = nativeOpen(authorizedFileDescriptor, frequencyHz, sampleRate)
        check(handle != 0L) { "Native RTL-SDR open returned an invalid handle" }
        return handle
    }

    fun readPcm(handle: Long, destination: ShortArray): Int = nativeReadPcm(handle, destination)

    fun description(handle: Long): String = nativeDescription(handle)

    fun close(handle: Long) {
        if (handle != 0L) nativeClose(handle)
    }

    private external fun nativeOpen(
        authorizedFileDescriptor: Int,
        frequencyHz: Long,
        sampleRate: Int,
    ): Long
    private external fun nativeReadPcm(handle: Long, destination: ShortArray): Int
    private external fun nativeDescription(handle: Long): String
    private external fun nativeClose(handle: Long)
}

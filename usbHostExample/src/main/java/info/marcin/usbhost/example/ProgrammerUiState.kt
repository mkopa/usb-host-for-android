package info.marcin.usbhost.example

import info.marcin.usbhost.ProgrammerInfo
import info.marcin.usbhost.StlinkDevice
import info.marcin.usbhost.TargetInfo

enum class OperationState {
    IDLE,
    REQUESTING_PERMISSION,
    CONNECTING,
    CONNECTED,
    READING,
    ERROR,
}

data class ProgrammerUiState(
    val devices: List<StlinkDevice> = emptyList(),
    val selectedDeviceId: Int? = null,
    val operation: OperationState = OperationState.IDLE,
    val programmer: ProgrammerInfo? = null,
    val target: TargetInfo? = null,
    val preview: List<HexLine> = emptyList(),
    val status: String = "Connect a probe through USB OTG, then scan.",
    val log: List<String> = listOf("Ready — read-only mode"),
) {
    val busy: Boolean
        get() = operation == OperationState.REQUESTING_PERMISSION ||
            operation == OperationState.CONNECTING || operation == OperationState.READING

    val connected: Boolean
        get() = operation == OperationState.CONNECTED || operation == OperationState.READING
}

data class HexLine(
    val address: Long,
    val hex: String,
    val ascii: String,
)

fun formatHexPreview(baseAddress: Long, bytes: ByteArray, width: Int = 16): List<HexLine> {
    require(width in 1..32) { "width must be between 1 and 32" }
    return bytes.asList().chunked(width).mapIndexed { line, chunk ->
        val hex = chunk.joinToString(" ") { "%02X".format(it.toInt() and 0xff) }
        val ascii = chunk.joinToString("") {
            val value = it.toInt() and 0xff
            if (value in 32..126) value.toChar().toString() else "."
        }
        HexLine(baseAddress + line * width, hex, ascii)
    }
}

package info.marcin.usbhost.example.rtlsdr

data class RtlSdrCandidate(
    val deviceId: Int,
    val vendorId: Int,
    val productId: Int,
    val name: String,
)

data class RtlSdrConnectionInfo(
    val model: String,
    val usbId: String,
    val usbVersion: String,
    val deviceRelease: String,
    val configuration: String,
    val interfaceNumber: Int,
    val endpoints: List<String>,
)

enum class RtlSdrLinkState {
    IDLE,
    REQUESTING_PERMISSION,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    ERROR,
}

data class RtlSdrUiState(
    val devices: List<RtlSdrCandidate> = emptyList(),
    val selectedDeviceId: Int? = null,
    val linkState: RtlSdrLinkState = RtlSdrLinkState.IDLE,
    val status: String = "Connect a common RTL2832U dongle through USB OTG.",
    val connection: RtlSdrConnectionInfo? = null,
    val log: List<String> = emptyList(),
) {
    val busy: Boolean
        get() = linkState == RtlSdrLinkState.REQUESTING_PERMISSION ||
            linkState == RtlSdrLinkState.CONNECTING ||
            linkState == RtlSdrLinkState.DISCONNECTING

    val connected: Boolean
        get() = linkState == RtlSdrLinkState.CONNECTED
}

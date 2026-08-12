package info.marcin.usbhost.example.rtlsdr

import android.app.Activity
import android.app.PendingIntent
import android.content.Intent
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.os.Build
import info.marcin.usbhost.transport.GenericUsbDevice
import info.marcin.usbhost.transport.GenericUsbInterface
import info.marcin.usbhost.transport.UsbTransportException
import java.io.Closeable
import java.util.Locale
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class RtlSdrController(
    private val activity: Activity,
    private val onState: (RtlSdrUiState) -> Unit,
) : Closeable {
    private val usbManager = activity.getSystemService(UsbManager::class.java)
    private val worker: ExecutorService = Executors.newSingleThreadExecutor { task ->
        Thread(task, "rtl-sdr-usb").apply { isDaemon = true }
    }
    private val stateLock = Any()
    private var state = RtlSdrUiState()
    private var androidConnection: UsbDeviceConnection? = null
    private var transportDevice: GenericUsbDevice? = null
    private var claimedInterface: GenericUsbInterface? = null
    private var nativeSession = 0L
    private var audioTrack: AudioTrack? = null
    @Volatile private var streamRequested = false
    @Volatile private var activeDeviceId: Int? = null

    fun scan() {
        val devices = usbManager.deviceList.values.mapNotNull { device ->
            knownModel(device)?.let { model ->
                RtlSdrCandidate(device.deviceId, device.vendorId, device.productId, model)
            }
        }.sortedBy { it.deviceId }
        update("Scan completed: ${devices.size} compatible dongle(s)") { current ->
            val selected = current.selectedDeviceId?.takeIf { id -> devices.any { it.deviceId == id } }
                ?: devices.firstOrNull()?.deviceId
            current.copy(
                devices = devices,
                selectedDeviceId = selected,
                status = when {
                    current.connected -> current.status
                    devices.isEmpty() -> "No supported RTL2832U dongle detected."
                    else -> "Found ${devices.size} compatible RTL-SDR dongle${if (devices.size == 1) "" else "s"}."
                },
            )
        }
    }

    fun select(candidate: RtlSdrCandidate) {
        update("Selected ${candidate.name} ${usbId(candidate.vendorId, candidate.productId)}") { current ->
            if (current.busy || current.connected) current else current.copy(selectedDeviceId = candidate.deviceId)
        }
    }

    fun requestPermissionOrConnect() {
        val snapshot = currentState()
        if (snapshot.busy || snapshot.connected) return
        val device = selectedUsbDevice(snapshot.selectedDeviceId)
        if (device == null || knownModel(device) == null) {
            update("Connection skipped: no compatible dongle selected") {
                it.copy(linkState = RtlSdrLinkState.ERROR, status = "Select an attached RTL-SDR dongle first.")
            }
            return
        }
        if (usbManager.hasPermission(device)) {
            connect(device)
            return
        }

        val permissionIntent = Intent(ACTION_USB_PERMISSION).setPackage(activity.packageName)
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) PendingIntent.FLAG_MUTABLE else 0
        val pending = PendingIntent.getBroadcast(activity, device.deviceId, permissionIntent, flags)
        update("Android USB permission requested") {
            it.copy(
                linkState = RtlSdrLinkState.REQUESTING_PERMISSION,
                status = "Waiting for Android USB permission…",
            )
        }
        usbManager.requestPermission(device, pending)
    }

    fun onPermissionResult(device: UsbDevice?, granted: Boolean) {
        if (!granted || device == null || knownModel(device) == null) {
            update("USB permission denied") {
                it.copy(linkState = RtlSdrLinkState.ERROR, status = "USB permission was not granted.")
            }
            return
        }
        connect(device)
    }

    fun disconnect(reason: String = "Disconnected safely.") {
        val snapshot = currentState()
        if (snapshot.linkState == RtlSdrLinkState.PLAYING ||
            snapshot.linkState == RtlSdrLinkState.TUNING) {
            stopRadio(reason)
            return
        }
        if (snapshot.busy || (!snapshot.connected && transportDevice == null)) return
        update("Closing interface and USB session") {
            it.copy(linkState = RtlSdrLinkState.DISCONNECTING, status = "Releasing interface 0…")
        }
        worker.execute {
            closeCurrent()
            update(reason) {
                it.copy(linkState = RtlSdrLinkState.IDLE, status = reason, connection = null)
            }
        }
    }

    fun onDetached(device: UsbDevice?) {
        if (device != null && device.deviceId == activeDeviceId) {
            streamRequested = false
            worker.execute {
                closeCurrent()
                update("Dongle detached; session closed") {
                    it.copy(
                        linkState = RtlSdrLinkState.IDLE,
                        status = "RTL-SDR dongle detached.",
                        connection = null,
                    )
                }
                scan()
            }
        } else {
            scan()
        }
    }

    fun play93_9Mhz() {
        val snapshot = currentState()
        if (snapshot.linkState != RtlSdrLinkState.CONNECTED || streamRequested) return
        val connection = androidConnection ?: return
        streamRequested = true
        update("Preparing rtl_fm WBFM profile for 93.9 MHz") {
            it.copy(
                linkState = RtlSdrLinkState.TUNING,
                status = "Initializing RTL2832U and tuner for 93.9 MHz…",
            )
        }
        worker.execute { streamRadio(connection) }
    }

    fun stopRadio(reason: String = "Radio stopped and USB session closed.") {
        if (!streamRequested && currentState().linkState != RtlSdrLinkState.TUNING) return
        streamRequested = false
        update("Stopping FM audio") {
            it.copy(linkState = RtlSdrLinkState.DISCONNECTING, status = reason)
        }
    }

    private fun connect(device: UsbDevice) {
        update("Opening authorized generic USB session") {
            it.copy(linkState = RtlSdrLinkState.CONNECTING, status = "Opening ${knownModel(device)}…")
        }
        worker.execute {
            var openedConnection: UsbDeviceConnection? = null
            var openedTransport: GenericUsbDevice? = null
            var openedInterface: GenericUsbInterface? = null
            try {
                closeCurrent()
                openedConnection = usbManager.openDevice(device)
                    ?: throw IllegalStateException("UsbManager.openDevice returned no connection")
                openedTransport = GenericUsbDevice.open(device, openedConnection)
                val descriptor = openedTransport.descriptor
                val configuration = openedTransport.activeConfiguration
                    ?: throw IllegalStateException("Dongle has no active USB configuration")
                val interfaceDescriptor = configuration.interfaces.firstOrNull { it.interfaceNumber == 0 }
                    ?: throw IllegalStateException("Dongle has no interface 0")
                openedInterface = openedTransport.claimInterface(0)
                val alternate = openedInterface.activeAlternateSetting

                androidConnection = openedConnection
                transportDevice = openedTransport
                claimedInterface = openedInterface
                activeDeviceId = device.deviceId

                val info = RtlSdrConnectionInfo(
                    model = requireNotNull(knownModel(device)),
                    usbId = usbId(descriptor.vendorId, descriptor.productId),
                    usbVersion = formatBcd(descriptor.usbVersionBcd),
                    deviceRelease = formatBcd(descriptor.deviceReleaseBcd),
                    configuration = "${configuration.configurationValue} · ${configuration.interfaces.size} interface(s)",
                    interfaceNumber = interfaceDescriptor.interfaceNumber,
                    endpoints = alternate.endpoints.map { endpoint ->
                        String.format(
                            Locale.ROOT,
                            "0x%02X  %s %s  max packet %d",
                            endpoint.address,
                            endpoint.direction.name,
                            endpoint.transferType.name,
                            endpoint.maximumPacketSize,
                        )
                    },
                )
                update("Connection established; interface 0 claimed") {
                    it.copy(
                        linkState = RtlSdrLinkState.CONNECTED,
                        status = "Connected — USB session open and interface 0 claimed.",
                        connection = info,
                    )
                }
            } catch (error: Throwable) {
                runCatching { openedInterface?.close() }
                runCatching { openedTransport?.close() }
                openedConnection?.close()
                val detail = when (error) {
                    is UsbTransportException -> "${error.status}: ${error.message}"
                    else -> error.message ?: error.javaClass.simpleName
                }
                update("Connection failed: $detail") {
                    it.copy(
                        linkState = RtlSdrLinkState.ERROR,
                        status = "Connection failed — $detail",
                        connection = null,
                    )
                }
            }
        }
    }

    private fun streamRadio(connection: UsbDeviceConnection) {
        try {
            releaseGenericSession()
            val opened = RtlSdrNative.open(
                connection.fileDescriptor,
                WbfmDemodulator.CAPTURE_FREQUENCY_HZ,
                WbfmDemodulator.CAPTURE_RATE,
            )
            nativeSession = opened
            if (!streamRequested) {
                finishRadio("Radio start cancelled.")
                return
            }

            val track = createAudioTrack()
            audioTrack = track
            track.play()
            val description = RtlSdrNative.description(opened)
            update("WBFM audio started: $description") {
                it.copy(
                    linkState = RtlSdrLinkState.PLAYING,
                    status = "Playing 93.9 MHz · mono FM · 32 kHz audio",
                )
            }

            val demodulator = WbfmDemodulator()
            val iq = ByteArray(IQ_BUFFER_BYTES)
            while (streamRequested) {
                val count = RtlSdrNative.read(opened, iq)
                if (count < 0) throw IllegalStateException("RTL-SDR bulk stream stopped")
                if (count == 0) continue
                val pcm = demodulator.process(iq, count)
                if (pcm.isNotEmpty()) {
                    val written = track.write(pcm, 0, pcm.size, AudioTrack.WRITE_BLOCKING)
                    if (written < 0) throw IllegalStateException("AudioTrack write failed: $written")
                }
            }
            finishRadio("Radio stopped and USB session closed.")
        } catch (error: Throwable) {
            val detail = error.message ?: error.javaClass.simpleName
            closeCurrent()
            update("FM audio failed: $detail") {
                it.copy(
                    linkState = RtlSdrLinkState.ERROR,
                    status = "FM audio failed — $detail",
                    connection = null,
                )
            }
        }
    }

    private fun createAudioTrack(): AudioTrack {
        val minimum = AudioTrack.getMinBufferSize(
            WbfmDemodulator.AUDIO_RATE,
            AudioFormat.CHANNEL_OUT_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
        )
        check(minimum > 0) { "AudioTrack does not support 32 kHz mono PCM" }
        val builder = AudioTrack.Builder()
            .setAudioAttributes(
                AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build(),
            )
            .setAudioFormat(
                AudioFormat.Builder()
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                    .setSampleRate(WbfmDemodulator.AUDIO_RATE)
                    .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                    .build(),
            )
            .setTransferMode(AudioTrack.MODE_STREAM)
            .setBufferSizeInBytes(maxOf(minimum * 4, AUDIO_BUFFER_BYTES))
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder.setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
        }
        return builder.build().also {
            check(it.state == AudioTrack.STATE_INITIALIZED) { "AudioTrack initialization failed" }
            it.setVolume(1.0f)
        }
    }

    private fun finishRadio(message: String) {
        closeRadioResources()
        androidConnection?.close()
        androidConnection = null
        activeDeviceId = null
        update(message) {
            it.copy(linkState = RtlSdrLinkState.IDLE, status = message, connection = null)
        }
        scan()
    }

    private fun releaseGenericSession() {
        val interfaceToClose = claimedInterface
        val deviceToClose = transportDevice
        claimedInterface = null
        transportDevice = null
        runCatching { interfaceToClose?.close() }
        runCatching { deviceToClose?.close() }
    }

    private fun closeRadioResources() {
        streamRequested = false
        val track = audioTrack
        audioTrack = null
        runCatching { track?.pause() }
        runCatching { track?.flush() }
        runCatching { track?.stop() }
        track?.release()
        val handle = nativeSession
        nativeSession = 0L
        runCatching { RtlSdrNative.close(handle) }
    }

    private fun closeCurrent() {
        streamRequested = false
        val connectionToClose = androidConnection
        androidConnection = null
        activeDeviceId = null
        releaseGenericSession()
        closeRadioResources()
        connectionToClose?.close()
    }

    private fun selectedUsbDevice(deviceId: Int?): UsbDevice? =
        deviceId?.let { id -> usbManager.deviceList.values.firstOrNull { it.deviceId == id } }

    private fun currentState(): RtlSdrUiState = synchronized(stateLock) { state }

    private fun update(logEntry: String? = null, transform: (RtlSdrUiState) -> RtlSdrUiState) {
        val next = synchronized(stateLock) {
            val transformed = transform(state)
            state = if (logEntry == null) transformed else transformed.copy(
                log = (listOf(logEntry) + transformed.log).take(MAX_LOG_ENTRIES),
            )
            state
        }
        activity.runOnUiThread { onState(next) }
    }

    override fun close() {
        worker.execute { closeCurrent() }
        worker.shutdown()
    }

    companion object {
        const val ACTION_USB_PERMISSION =
            "info.marcin.usbhost.example.rtlsdr.action.USB_PERMISSION"
        private const val MAX_LOG_ENTRIES = 16
        private const val IQ_BUFFER_BYTES = 16 * 16_384
        private const val AUDIO_BUFFER_BYTES = 64 * 1024

        private val models = mapOf(
            (0x0bda shl 16 or 0x2832) to "Generic RTL2832U",
            (0x0bda shl 16 or 0x2838) to "Generic RTL2832U OEM",
        )

        private fun knownModel(device: UsbDevice): String? =
            models[device.vendorId shl 16 or device.productId]

        private fun usbId(vendorId: Int, productId: Int): String =
            String.format(Locale.ROOT, "%04X:%04X", vendorId, productId)

        private fun formatBcd(value: Int): String = String.format(
            Locale.ROOT,
            "%X.%02X",
            value ushr 8,
            value and 0xff,
        )
    }
}

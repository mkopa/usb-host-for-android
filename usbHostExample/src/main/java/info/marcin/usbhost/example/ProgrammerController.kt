package info.marcin.usbhost.example

import android.app.Activity
import android.app.PendingIntent
import android.content.Intent
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import info.marcin.usbhost.StlinkDevice
import info.marcin.usbhost.StlinkException
import info.marcin.usbhost.StlinkProber
import info.marcin.usbhost.StlinkSession
import java.io.Closeable
import java.util.Locale
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class ProgrammerController(
    private val activity: Activity,
    private val onState: (ProgrammerUiState) -> Unit,
) : Closeable {
    private val usbManager = activity.getSystemService(UsbManager::class.java)
    private val worker: ExecutorService = Executors.newSingleThreadExecutor { task ->
        Thread(task, "usb-host-programmer").apply { isDaemon = true }
    }
    private var state = ProgrammerUiState()
    private var session: StlinkSession? = null

    fun scan() {
        val devices = StlinkProber.findAll(usbManager)
        val selection = state.selectedDeviceId?.takeIf { selected ->
            devices.any { it.deviceId == selected }
        } ?: devices.firstOrNull()?.deviceId
        update(
            state.copy(
                devices = devices,
                selectedDeviceId = selection,
                status = if (devices.isEmpty()) {
                    "No compatible STLINK-V3 probe found."
                } else {
                    "Found ${devices.size} compatible probe${if (devices.size == 1) "" else "s"}."
                },
            ),
            "Scan completed: ${devices.size} compatible probe(s)",
        )
    }

    fun select(device: StlinkDevice) {
        if (state.busy || state.connected) return
        update(state.copy(selectedDeviceId = device.deviceId), "Selected ${device.label()}")
    }

    fun requestPermissionOrConnect() {
        val device = state.devices.firstOrNull { it.deviceId == state.selectedDeviceId }
        if (device == null) {
            update(state.copy(status = "Select an attached probe first."), "Connect skipped: no probe selected")
            return
        }
        if (usbManager.hasPermission(device.usbDevice)) {
            connect(device)
            return
        }

        val permissionIntent = Intent(ACTION_USB_PERMISSION).setPackage(activity.packageName)
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) PendingIntent.FLAG_MUTABLE else 0
        val pending = PendingIntent.getBroadcast(activity, device.deviceId, permissionIntent, flags)
        update(
            state.copy(
                operation = OperationState.REQUESTING_PERMISSION,
                status = "Waiting for Android USB permission…",
            ),
            "USB permission requested",
        )
        usbManager.requestPermission(device.usbDevice, pending)
    }

    fun onPermissionResult(device: UsbDevice?, granted: Boolean) {
        if (!granted || device == null) {
            update(
                state.copy(operation = OperationState.IDLE, status = "USB permission was not granted."),
                "USB permission denied",
            )
            return
        }
        val selected = state.devices.firstOrNull { it.deviceId == device.deviceId }
            ?: StlinkDevice(device)
        connect(selected)
    }

    fun readPreview() {
        val active = session ?: return
        val target = state.target ?: return
        if (state.busy) return
        update(state.copy(operation = OperationState.READING, status = "Reading 256 bytes…"), "Read started")
        worker.execute {
            try {
                val bytes = active.readMemory(target.flashBase, PREVIEW_BYTES)
                update(
                    state.copy(
                        operation = OperationState.CONNECTED,
                        preview = formatHexPreview(target.flashBase, bytes),
                        status = "Read-only preview refreshed.",
                    ),
                    "Read completed: $PREVIEW_BYTES bytes at ${hex32(target.flashBase)}",
                )
            } catch (error: StlinkException) {
                fail("Read failed", error)
            } catch (error: RuntimeException) {
                fail("Read failed", error)
            }
        }
    }

    fun disconnect(reason: String = "Disconnected") {
        if (state.operation == OperationState.IDLE && session == null) return
        worker.execute {
            closeSession()
            update(
                state.copy(
                    operation = OperationState.IDLE,
                    programmer = null,
                    target = null,
                    preview = emptyList(),
                    status = reason,
                ),
                reason,
            )
        }
    }

    fun onDetached(device: UsbDevice?) {
        if (device != null && device.deviceId == state.selectedDeviceId) {
            disconnect("Probe detached — session closed safely.")
        }
        scan()
    }

    private fun connect(device: StlinkDevice) {
        if (state.busy || state.connected) return
        update(
            state.copy(operation = OperationState.CONNECTING, status = "Opening ${device.label()}…"),
            "Opening read-only session",
        )
        worker.execute {
            try {
                closeSession()
                val opened = device.open(usbManager)
                val target = opened.connectTarget()
                session = opened
                update(
                    state.copy(
                        operation = OperationState.CONNECTED,
                        programmer = opened.programmerInfo,
                        target = target,
                        preview = emptyList(),
                        status = "Connected — target operations are read-only.",
                    ),
                    "Connected: chip ${hex(target.chipId, 3)}, ${target.targetVoltageMv} mV",
                )
            } catch (error: StlinkException) {
                fail("Connection failed", error)
            } catch (error: RuntimeException) {
                fail("Connection failed", error)
            }
        }
    }

    private fun fail(prefix: String, error: Throwable) {
        closeSession()
        val detail = if (error is StlinkException) "${error.status}: ${error.message}" else error.message
        update(
            state.copy(
                operation = OperationState.ERROR,
                programmer = null,
                target = null,
                preview = emptyList(),
                status = "$prefix — ${detail ?: "unknown error"}",
            ),
            "$prefix: ${detail ?: error.javaClass.simpleName}",
        )
    }

    private fun closeSession() {
        val active = session
        session = null
        active?.close()
    }

    private fun update(next: ProgrammerUiState, logEntry: String? = null) {
        state = if (logEntry == null) next else next.copy(
            log = (listOf(logEntry) + next.log).take(MAX_LOG_ENTRIES),
        )
        activity.runOnUiThread { onState(state) }
    }

    override fun close() {
        closeSession()
        worker.shutdownNow()
    }

    companion object {
        const val ACTION_USB_PERMISSION = "info.marcin.usbhost.example.action.USB_PERMISSION"
        private const val PREVIEW_BYTES = 256
        private const val MAX_LOG_ENTRIES = 20
    }
}

private fun StlinkDevice.label(): String = String.format(
    Locale.ROOT,
    "STLINK-V3  %04X:%04X",
    vendorId,
    productId,
)

fun hex(value: Long, width: Int): String = "0x%0${width}X".format(Locale.ROOT, value)

fun hex32(value: Long): String = hex(value, 8)

package info.marcin.usbhost.example

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.core.content.ContextCompat
import info.marcin.usbhost.example.ui.theme.UsbHostProgrammerTheme

class MainActivity : ComponentActivity() {
    private var uiState by mutableStateOf(ProgrammerUiState())
    private lateinit var controller: ProgrammerController

    private val usbEvents = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            when (intent?.action) {
                ProgrammerController.ACTION_USB_PERMISSION -> controller.onPermissionResult(
                    intent.usbDevice(),
                    intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false),
                )
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> controller.scan()
                UsbManager.ACTION_USB_DEVICE_DETACHED -> controller.onDetached(intent.usbDevice())
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        controller = ProgrammerController(this) { uiState = it }
        registerUsbEvents()
        setContent {
            UsbHostProgrammerTheme {
                ProgrammerScreen(
                    state = uiState,
                    onScan = controller::scan,
                    onSelect = controller::select,
                    onConnect = controller::requestPermissionOrConnect,
                    onRead = controller::readPreview,
                    onDisconnect = { controller.disconnect() },
                )
            }
        }
        controller.scan()
    }

    private fun registerUsbEvents() {
        val filter = IntentFilter().apply {
            addAction(ProgrammerController.ACTION_USB_PERMISSION)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        ContextCompat.registerReceiver(this, usbEvents, filter, ContextCompat.RECEIVER_NOT_EXPORTED)
    }

    override fun onDestroy() {
        unregisterReceiver(usbEvents)
        controller.close()
        super.onDestroy()
    }
}

@Suppress("DEPRECATION")
private fun Intent.usbDevice(): UsbDevice? = if (Build.VERSION.SDK_INT >= 33) {
    getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice::class.java)
} else {
    getParcelableExtra(UsbManager.EXTRA_DEVICE)
}

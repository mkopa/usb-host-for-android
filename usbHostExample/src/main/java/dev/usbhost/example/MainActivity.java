package dev.usbhost.example;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import dev.usbhost.android.ProgrammerInfo;
import dev.usbhost.android.StlinkDevice;
import dev.usbhost.android.StlinkException;
import dev.usbhost.android.StlinkProber;
import dev.usbhost.android.StlinkSession;
import dev.usbhost.android.TargetInfo;

/** Minimal non-destructive validation client. Memory contents are never displayed or logged. */
public final class MainActivity extends Activity {
    private static final String ACTION_USB_PERMISSION =
            "dev.usbhost.example.action.USB_PERMISSION";
    private static final int[] READ_VALIDATION_SIZES = {1, 4, 1024, 64 * 1024};
    private static final int READ_VALIDATION_REPETITIONS = 20;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private TextView statusView;
    private UsbManager usbManager;
    private StlinkSession session;

    private final BroadcastReceiver permissionReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!ACTION_USB_PERMISSION.equals(intent.getAction())) return;
            UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
            if (granted && device != null) {
                runSession(new StlinkDevice(device));
            } else {
                show("USB permission denied");
            }
        }
    };

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = (int) (24 * getResources().getDisplayMetrics().density);
        layout.setPadding(padding, padding, padding, padding);

        statusView = new TextView(this);
        statusView.setText(R.string.ready);
        Button button = new Button(this);
        button.setText(R.string.run_probe);
        button.setOnClickListener(this::probe);
        layout.addView(statusView);
        layout.addView(button);
        setContentView(layout);

        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        ContextCompat.registerReceiver(
                this,
                permissionReceiver,
                filter,
                ContextCompat.RECEIVER_NOT_EXPORTED);
    }

    private void probe(View ignored) {
        List<StlinkDevice> devices = StlinkProber.findAll(usbManager);
        if (devices.isEmpty()) {
            show("No supported ST-Link V3 debug device found");
            return;
        }
        StlinkDevice device = devices.get(0);
        if (usbManager.hasPermission(device.getUsbDevice())) {
            runSession(device);
            return;
        }
        Intent permissionIntent = new Intent(ACTION_USB_PERMISSION).setPackage(getPackageName());
        int flags = Build.VERSION.SDK_INT >= 31 ? PendingIntent.FLAG_MUTABLE : 0;
        PendingIntent pending = PendingIntent.getBroadcast(this, 0, permissionIntent, flags);
        usbManager.requestPermission(device.getUsbDevice(), pending);
        show("Waiting for Android USB permission");
    }

    private void runSession(StlinkDevice device) {
        show("Opening programmer…");
        executor.execute(() -> {
            try {
                closeSession();
                session = device.open(usbManager);
                ProgrammerInfo programmer = session.getProgrammerInfo();
                TargetInfo target = session.connectTarget();
                int verifiedReads = validateFlashReads(session, target.getFlashBase());
                show(String.format(Locale.ROOT,
                        "ST-Link V%d JTAG %d; target 0x%03X; %d mV; flash %d KiB; "
                                + "SRAM %d KiB; %d stable read-only reads",
                        programmer.getStlinkVersion(), programmer.getJtagVersion(),
                        target.getChipId(), target.getTargetVoltageMv(),
                        target.getFlashSize() / 1024, target.getSramSize() / 1024,
                        verifiedReads));
            } catch (StlinkException error) {
                show(error.getStatus() + ": " + error.getMessage());
            } catch (RuntimeException error) {
                show("Read-only validation failed: " + error.getMessage());
            } finally {
                closeSession();
            }
        });
    }

    private static int validateFlashReads(StlinkSession activeSession, long flashBase)
            throws StlinkException {
        int completed = 0;
        for (int size : READ_VALIDATION_SIZES) {
            byte[] expectedHash = null;
            for (int repetition = 0; repetition < READ_VALIDATION_REPETITIONS; repetition++) {
                byte[] currentHash = sha256(activeSession.readMemory(flashBase, size));
                if (expectedHash == null) {
                    expectedHash = currentHash;
                } else if (!Arrays.equals(expectedHash, currentHash)) {
                    throw new IllegalStateException("flash read changed for size " + size);
                }
                completed++;
            }
        }
        return completed;
    }

    private static byte[] sha256(byte[] data) {
        try {
            return MessageDigest.getInstance("SHA-256").digest(data);
        } catch (NoSuchAlgorithmException impossibleOnAndroid) {
            throw new AssertionError("SHA-256 is unavailable", impossibleOnAndroid);
        }
    }

    private void show(String message) {
        runOnUiThread(() -> statusView.setText(message));
    }

    private void closeSession() {
        StlinkSession current = session;
        session = null;
        if (current != null) current.close();
    }

    @Override
    protected void onDestroy() {
        unregisterReceiver(permissionReceiver);
        executor.execute(this::closeSession);
        executor.shutdown();
        super.onDestroy();
    }
}

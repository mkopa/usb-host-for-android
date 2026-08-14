package info.marcin.usbhost.example.rtlsdr

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RtlSdrScreen(
    state: RtlSdrUiState,
    onScan: () -> Unit,
    onSelect: (RtlSdrCandidate) -> Unit,
    onConnect: () -> Unit,
    onDisconnect: () -> Unit,
    onPlay: () -> Unit,
    onStop: () -> Unit,
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text("RtlSdrForAndroid", fontWeight = FontWeight.Bold)
                        Text(
                            "USB HOST · RTL_FM WBFM AUDIO",
                            color = MaterialTheme.colorScheme.primary,
                            style = MaterialTheme.typography.labelSmall,
                        )
                    }
                },
            )
        },
    ) { padding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .background(
                    Brush.verticalGradient(
                        listOf(
                            MaterialTheme.colorScheme.surface,
                            MaterialTheme.colorScheme.surfaceContainer,
                        ),
                    ),
                )
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item { Spacer(Modifier.height(4.dp)) }
            item { StatusCard(state) }
            item {
                SectionTitle("RTL-SDR DONGLES", "${state.devices.size} detected")
                Card(Modifier.fillMaxWidth(), shape = RoundedCornerShape(18.dp)) {
                    Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        if (state.devices.isEmpty()) {
                            Text(
                                "Connect a generic RTL2832U dongle (0BDA:2832 or 0BDA:2838).",
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                        }
                        state.devices.forEach { device ->
                            FilterChip(
                                selected = device.deviceId == state.selectedDeviceId,
                                onClick = { onSelect(device) },
                                enabled = !state.busy && !state.connected,
                                label = {
                                    Text(
                                        "%s · %04X:%04X".format(
                                            Locale.ROOT,
                                            device.name,
                                            device.vendorId,
                                            device.productId,
                                        ),
                                    )
                                },
                            )
                        }
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            OutlinedButton(onClick = onScan, enabled = !state.busy) { Text("Scan") }
                            if (state.connected) {
                                OutlinedButton(onClick = onDisconnect, enabled = !state.busy) {
                                    Text("Disconnect")
                                }
                            } else {
                                Button(
                                    onClick = onConnect,
                                    enabled = state.selectedDeviceId != null && !state.busy,
                                ) {
                                    Text(if (state.busy) "Working…" else "Connect")
                                }
                            }
                        }
                    }
                }
            }
            state.connection?.let { connection ->
                item {
                    SectionTitle("USB SESSION", "interface 0 claimed")
                    ConnectionCard(connection)
                }
                item {
                    SectionTitle("FM RADIO", "rtl_fm WBFM profile")
                    Card(
                        Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = Color(0xFF13A879).copy(alpha = 0.12f),
                        ),
                        shape = RoundedCornerShape(18.dp),
                    ) {
                        Column(
                            Modifier.padding(16.dp),
                            verticalArrangement = Arrangement.spacedBy(10.dp),
                        ) {
                            Text("93.9 MHz", style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.Bold)
                            Text("Wide FM · 170 kHz discriminator · 32 kHz mono audio")
                            if (state.playing) {
                                Button(onClick = onStop) { Text("Stop audio") }
                            } else {
                                Button(onClick = onPlay, enabled = !state.busy) {
                                    Text(if (state.linkState == RtlSdrLinkState.TUNING) "Tuning…" else "Play 93.9 MHz")
                                }
                            }
                        }
                    }
                }
            }
            item {
                SectionTitle("SAFETY SCOPE", "connection prototype")
                Card(
                    Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.08f),
                    ),
                ) {
                    Text(
                        "The connection probe is inspection-only. Starting FM writes volatile " +
                            "RTL2832U/R82xx registers and streams IQ; EEPROM is left unchanged.",
                        Modifier.padding(14.dp),
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            }
            item {
                SectionTitle("SESSION LOG", "latest first")
                Card(Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        if (state.log.isEmpty()) Text("No operations yet.")
                        state.log.forEachIndexed { index, entry ->
                            Text(
                                if (index == 0) "●  $entry" else "·  $entry",
                                style = MaterialTheme.typography.bodySmall,
                                color = if (index == 0) MaterialTheme.colorScheme.onSurface
                                else MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                        }
                    }
                }
            }
            item { Spacer(Modifier.height(20.dp)) }
        }
    }
}

@Composable
private fun StatusCard(state: RtlSdrUiState) {
    val accent = when (state.linkState) {
        RtlSdrLinkState.CONNECTED, RtlSdrLinkState.PLAYING -> Color(0xFF13A879)
        RtlSdrLinkState.ERROR -> MaterialTheme.colorScheme.error
        RtlSdrLinkState.CONNECTING,
        RtlSdrLinkState.TUNING,
        RtlSdrLinkState.REQUESTING_PERMISSION,
        RtlSdrLinkState.DISCONNECTING -> Color(0xFFE09A25)
        else -> MaterialTheme.colorScheme.primary
    }
    Card(
        Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = accent.copy(alpha = 0.12f)),
        shape = RoundedCornerShape(18.dp),
    ) {
        Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.width(6.dp).height(44.dp).background(accent, RoundedCornerShape(6.dp)))
            Spacer(Modifier.width(12.dp))
            Column {
                Text(state.linkState.name.replace('_', ' '), color = accent, fontWeight = FontWeight.Bold)
                Text(state.status, style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}

@Composable
private fun ConnectionCard(info: RtlSdrConnectionInfo) {
    Card(Modifier.fillMaxWidth(), shape = RoundedCornerShape(18.dp)) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(9.dp)) {
            Fact("Model", info.model)
            Fact("USB ID", info.usbId)
            Fact("USB version", info.usbVersion)
            Fact("Device release", info.deviceRelease)
            Fact("Configuration", info.configuration)
            Fact("Claimed interface", info.interfaceNumber.toString())
            Text("ENDPOINTS", style = MaterialTheme.typography.labelSmall,
                color = MaterialTheme.colorScheme.primary, fontWeight = FontWeight.Bold)
            if (info.endpoints.isEmpty()) Text("No endpoints in active alternate setting")
            info.endpoints.forEach { endpoint ->
                Text(endpoint, fontFamily = FontFamily.Monospace,
                    style = MaterialTheme.typography.bodySmall)
            }
        }
    }
}

@Composable
private fun Fact(label: String, value: String) {
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Text(value, fontWeight = FontWeight.SemiBold)
    }
}

@Composable
private fun SectionTitle(title: String, detail: String) {
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically) {
        Text(title, style = MaterialTheme.typography.labelLarge, fontWeight = FontWeight.Bold)
        Text(detail, style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}

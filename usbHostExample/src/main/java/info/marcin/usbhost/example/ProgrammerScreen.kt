package info.marcin.usbhost.example

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
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
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
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
import info.marcin.usbhost.StlinkDevice
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ProgrammerScreen(
    state: ProgrammerUiState,
    onScan: () -> Unit,
    onSelect: (StlinkDevice) -> Unit,
    onConnect: () -> Unit,
    onRead: () -> Unit,
    onDisconnect: () -> Unit,
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text("USB Host Programmer", fontWeight = FontWeight.Bold)
                        Text("STLINK-V3 · SAFE READ-ONLY", style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.primary)
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
                        listOf(MaterialTheme.colorScheme.surface, MaterialTheme.colorScheme.surfaceContainer),
                    ),
                )
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item { Spacer(Modifier.height(4.dp)) }
            item { StatusCard(state) }
            item {
                SectionTitle("PROBES", "${state.devices.size} attached")
                Card(Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        if (state.devices.isEmpty()) {
                            Text("Connect a compatible STLINK-V3 probe through Android USB OTG.")
                        }
                        state.devices.forEach { device ->
                            FilterChip(
                                selected = device.deviceId == state.selectedDeviceId,
                                onClick = { onSelect(device) },
                                enabled = !state.busy && !state.connected,
                                label = {
                                    Text(String.format(Locale.ROOT, "STLINK-V3  ·  %04X:%04X",
                                        device.vendorId, device.productId))
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
                                ) { Text(if (state.busy) "Working…" else "Connect") }
                            }
                        }
                    }
                }
            }
            if (state.programmer != null && state.target != null) {
                item {
                    SectionTitle("TARGET", "live session")
                    FactsCard(state)
                }
                item {
                    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically) {
                        SectionTitle("MEMORY PREVIEW", "256 bytes")
                        Button(onClick = onRead, enabled = !state.busy) { Text("Read flash") }
                    }
                }
                if (state.preview.isEmpty()) {
                    item {
                        EmptyPreview()
                    }
                } else {
                    items(state.preview) { line -> HexRow(line) }
                }
            }
            item {
                SectionTitle("SESSION LOG", "latest first")
                Card(Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        state.log.forEachIndexed { index, message ->
                            Text(
                                text = if (index == 0) "●  $message" else "·  $message",
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
private fun StatusCard(state: ProgrammerUiState) {
    val accent = when (state.operation) {
        OperationState.CONNECTED, OperationState.READING -> Color(0xFF1EA672)
        OperationState.ERROR -> MaterialTheme.colorScheme.error
        OperationState.CONNECTING, OperationState.REQUESTING_PERMISSION -> Color(0xFFE09A25)
        else -> MaterialTheme.colorScheme.primary
    }
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = accent.copy(alpha = 0.12f)),
        shape = RoundedCornerShape(18.dp),
    ) {
        Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.width(6.dp).height(44.dp).background(accent, RoundedCornerShape(6.dp)))
            Spacer(Modifier.width(12.dp))
            Column {
                Text(state.operation.name.replace('_', ' '), fontWeight = FontWeight.Bold, color = accent)
                Text(state.status, style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}

@Composable
private fun FactsCard(state: ProgrammerUiState) {
    val programmer = requireNotNull(state.programmer)
    val target = requireNotNull(state.target)
    Card(Modifier.fillMaxWidth(), shape = RoundedCornerShape(18.dp)) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Fact("Probe firmware", "ST-Link V${programmer.stlinkVersion} · JTAG ${programmer.jtagVersion}")
            Fact("Target family", target.family)
            Fact("Device ID", hex(target.chipId, 3))
            Fact("Target voltage", "%.3f V".format(Locale.ROOT, target.targetVoltageMv / 1000.0))
            Fact("Flash", "${target.flashSize / 1024} KiB at ${hex32(target.flashBase)}")
            Fact("Flash page", "${target.flashPageSize} bytes")
            Fact("SRAM", "${target.sramSize / 1024} KiB at ${hex32(target.sramBase)}")
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
private fun EmptyPreview() {
    Surface(Modifier.fillMaxWidth(), color = MaterialTheme.colorScheme.surfaceContainerHigh,
        shape = RoundedCornerShape(14.dp)) {
        Text("Read a bounded flash preview. No write, erase, reset, or halt command is available.",
            Modifier.padding(16.dp), color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}

@Composable
private fun HexRow(line: HexLine) {
    Row(
        Modifier.fillMaxWidth().background(MaterialTheme.colorScheme.surfaceContainerHigh,
            RoundedCornerShape(6.dp)).padding(horizontal = 10.dp, vertical = 5.dp),
    ) {
        Text(hex32(line.address), fontFamily = FontFamily.Monospace,
            color = MaterialTheme.colorScheme.primary, style = MaterialTheme.typography.labelSmall)
        Spacer(Modifier.width(12.dp))
        Text(line.hex.padEnd(47), fontFamily = FontFamily.Monospace,
            style = MaterialTheme.typography.labelSmall, modifier = Modifier.weight(1f))
        Text(line.ascii, fontFamily = FontFamily.Monospace,
            color = MaterialTheme.colorScheme.onSurfaceVariant, style = MaterialTheme.typography.labelSmall)
    }
}

@Composable
private fun SectionTitle(title: String, detail: String) {
    Row(Modifier.fillMaxWidth().padding(top = 4.dp), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(title, style = MaterialTheme.typography.labelLarge, fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.primary)
        Text(detail, style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}

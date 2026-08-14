package info.marcin.usbhost.example.rtlsdr

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val DarkColors = darkColorScheme(
    primary = Color(0xFF55D9F4),
    secondary = Color(0xFF64DCAD),
    surface = Color(0xFF07151A),
    surfaceContainer = Color(0xFF102229),
)

private val LightColors = lightColorScheme(
    primary = Color(0xFF00677A),
    secondary = Color(0xFF006C50),
    surface = Color(0xFFF7FAFB),
    surfaceContainer = Color(0xFFEAF2F4),
)

@Composable
fun RtlSdrTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = if (isSystemInDarkTheme()) DarkColors else LightColors,
        content = content,
    )
}

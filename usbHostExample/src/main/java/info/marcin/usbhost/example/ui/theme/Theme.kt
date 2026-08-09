package info.marcin.usbhost.example.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext

private val DarkColors = darkColorScheme(
    primary = Color(0xFF62D6FF),
    secondary = Color(0xFF7EE2B8),
    surface = Color(0xFF0D141A),
    surfaceContainer = Color(0xFF151F27),
)

private val LightColors = lightColorScheme(
    primary = Color(0xFF006783),
    secondary = Color(0xFF006C4E),
    surface = Color(0xFFF8FAFC),
    surfaceContainer = Color(0xFFEDF2F5),
)

@Composable
fun UsbHostProgrammerTheme(content: @Composable () -> Unit) {
    val dark = isSystemInDarkTheme()
    val context = LocalContext.current
    val colors = when {
        Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && dark -> dynamicDarkColorScheme(context)
        Build.VERSION.SDK_INT >= Build.VERSION_CODES.S -> dynamicLightColorScheme(context)
        dark -> DarkColors
        else -> LightColors
    }
    MaterialTheme(colorScheme = colors, content = content)
}

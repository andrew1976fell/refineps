/*
 * Theme.kt — Material3 theme for the RefinePS app
 *
 * Uses dynamic color (Material You) on API 31+ — the system wallpaper
 * generates the color scheme automatically. Falls back to a static
 * blue/teal palette on older devices. Dark mode follows the system setting.
 *
 * To change the static palette, edit DarkColorScheme and LightColorScheme.
 * To disable dynamic color entirely, set dynamicColor = false in RefinePSTheme.
 */
package com.andrewscrap.refineps.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalContext

private val DarkColorScheme = darkColorScheme(
    primary = androidx.compose.ui.graphics.Color(0xFF90CAF9),
    secondary = androidx.compose.ui.graphics.Color(0xFF80CBC4),
    tertiary = androidx.compose.ui.graphics.Color(0xFFFFCC80)
)

private val LightColorScheme = lightColorScheme(
    primary = androidx.compose.ui.graphics.Color(0xFF1976D2),
    secondary = androidx.compose.ui.graphics.Color(0xFF00897B),
    tertiary = androidx.compose.ui.graphics.Color(0xFFF57C00)
)

@Composable
fun RefinePSTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    dynamicColor: Boolean = true,
    content: @Composable () -> Unit
) {
    val colorScheme = when {
        dynamicColor && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S -> {
            val context = LocalContext.current
            if (darkTheme) dynamicDarkColorScheme(context) else dynamicLightColorScheme(context)
        }
        darkTheme -> DarkColorScheme
        else -> LightColorScheme
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = Typography(),
        content = content
    )
}

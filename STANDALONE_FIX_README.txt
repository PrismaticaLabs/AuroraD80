AURORA D80 - Standalone Fix

Problema corregido:
El proyecto JUCE solo declaraba buildAU,buildVST3. Por eso Xcode no tenia un target ejecutable Standalone y Product > Run aparecia deshabilitado.

Cambio realizado:
pluginFormats="buildAU,buildVST3,buildStandalone"

En macOS:
1. Doble click en REGENERATE_XCODE.command
2. El script usa Projucer para regenerar Builds/MacOS/AuroraD80.xcodeproj
3. Xcode se abre automaticamente
4. Selecciona el scheme "AuroraD80 - Standalone Plugin" / "AuroraD80 - Standalone" y My Mac
5. Cmd+R

Si macOS bloquea el .command por seguridad: clic derecho > Open.

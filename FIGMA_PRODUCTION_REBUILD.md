# AURORA D80 — Figma Production Rebuild

## Qué cambió

- Canvas maestro: 1536 x 1024 (coordenadas originales de Figma).
- Tamaño inicial del plugin: 1152 x 768 (75%).
- Resize libre por esquina: deshabilitado en esta build para evitar drift entre artwork y controles.
- Los paneles principales se cargan desde exports reales del nodo aprobado de Figma, guardados en `Resources/`:
  - chassis
  - input/output meter (assets originales preservados; no se reemplazan)
  - central display
  - Delay Core
  - Tone Shaping + Stereo
  - Modulation + Character
  - Utility Bar
  - Preset Browser
- Los knobs y parámetros JUCE siguen siendo funcionales y se colocan en las coordenadas del frame aprobado.
- Los meters conservan el shell de Figma y sus LEDs se repintan con niveles reales.
- El readout del display se actualiza con el delay time real.

## Primera ejecución

1. Doble clic en `REGENERATE_XCODE.command`.
2. El script valida los exports aprobados guardados en `Resources/`, incluidas dimensiones y protección de integridad de ambos meters.
3. Projucer vuelve a generar Xcode e incrusta esos assets como BinaryData.
4. Se abre `Builds/MacOSX/AuroraD80.xcodeproj`.
5. Para instalar el AU, ejecuta `BUILD_INSTALL_AU.command`.

IMPORTANTE: el rebuild no depende de URLs temporales de Figma y no descarga archivos. Si la validación falla, restaura el asset aprobado correspondiente antes de regenerar Xcode. Los archivos `input_meter.png` y `output_meter.png` están protegidos para impedir cambios accidentales.

## Filosofía de escala

No se usa 1536x1024 como tamaño inicial. 1536x1024 es únicamente el sistema de coordenadas master. La UI visible usa 75% = 1152x768. Las futuras escalas discretas deben cambiar el tamaño completo del editor, no permitir deformación libre.

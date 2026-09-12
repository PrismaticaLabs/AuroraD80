# AURORA D80 — Integrated Development Build

Base visual maestra: Figma Production Candidate v1.4, 1536 × 1024.

## Implementado
- AU, VST3 y Standalone en el .jucer.
- GUI escalable manteniendo relación 3:2 y coordenadas de diseño 1536×1024.
- Input/Output meters L/R.
- Input/Output Trim.
- Display central animado, tiempo, sync y quality.
- Delay Core: Time, Sync, division, Ping-Pong, Feedback, Mix.
- Tone/Stereo: Low Cut, High Cut, Width.
- Modulation: Depth, Rate, Drift, Shape.
- Character: Drive, Evolve, Bloom.
- Utilities: Mode, Quality, Freeze, Oversampling, Bypass.
- Presets integrados, navegación, random Dice y A/B snapshots (Shift-click A/B para guardar; click para recuperar).
- Estado completo guardado/restaurado por APVTS.
- Scripts para regenerar Xcode e instalar AU.

## DSP
- Delay estéreo con interpolación lineal y host-sync.
- HP/LP en wet path.
- Modulación Sine/Triangle/Random + Drift.
- Width mid/side del wet.
- Drive + modos Digital/Tape/Analog.
- Evolve como softening/degradation character.
- Bloom como cross-feedback/diffusion ligera.
- Ping-Pong cross feedback.
- Freeze preserva el buffer con feedback cercano a unidad.
- Bypass conserva señal directa.

## Abrir en Xcode
Doble click `REGENERATE_XCODE.command` y selecciona el scheme Standalone o AU.

## Instalar Audio Unit
Doble click `BUILD_INSTALL_AU.command`.

## Nota de ingeniería
Esta es una versión integrada funcional de desarrollo. La verificación final de compilación AU/VST3/Standalone debe hacerse en macOS/Xcode, ya que el entorno donde se preparó este paquete no ejecuta Xcode. El oversampling tiene parámetro/UI persistente y queda preparado para sustituirse por una etapa `juce::dsp::Oversampling` dedicada en la etapa de optimización DSP; no se presenta como oversampling DSP validado todavía.

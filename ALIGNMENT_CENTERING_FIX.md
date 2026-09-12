# AURORA D80 — Internal Alignment + Centering Fix

This build corrects the systematic coordinate-space error identified after the 75% fidelity pass.

## Root cause
Figma's editable production structure (`6512:5806`) is positioned at **(-4,+6)** inside the root frame, while the final applied knob integration (`6512:6748`) is positioned at **(+4,-6)**. The previous JUCE build used the child coordinates without applying those parent transforms.

That caused, relative to the module artwork:
- knobs to sit **8 px too far left**;
- knobs to sit **12 px too low**;
- Sync / Ping-Pong / Shape hit areas to inherit the same drift;
- Preset dynamic content to be **4 px right / 6 px high**.

## Corrected
- Module artwork uses true root-space positions.
- Applied knobs use true root-space positions.
- Every label and value is centred on the *actual visual centre* of its knob asset.
- Delay Core, Tone/Stereo, Modulation/Character controls now share consistent vertical axes.
- Sync, division, Ping-Pong and Shape use root-correct hit areas.
- Preset bar artwork and live text/actions use the same root coordinate system.
- Scale numbers are shifted into the same root coordinate system as the applied knobs.

Master: **1536×1024**. Default editor: **1152×768 (75%)**.

# AURORA D80 — Figma Integration Pass 1

Reference: Figma node `6512:5804` — **AURORA D80 / Production Candidate v1.4**.
Master design size: **1536 x 1024**.

## Implemented in this pass
- JUCE editor changed from 800 x 420 to 1536 x 1024.
- Direct Figma-space coordinates used for the main chassis layout.
- Input and Output meters aligned to Figma: 174 x 685.
- Central display aligned to Figma: 725 x 207.
- Delay Core, Tone Shaping/Stereo, Modulation and Character footprints aligned to the approved frame.
- Existing functional JUCE controls repositioned to the applied knob coordinates from the Figma frame.
- Preset browser moved to the Figma footer footprint.
- Utility bar footprint added as a visual shell only.
- Existing DSP/APVTS attachments preserved.

## Existing DSP mapped into the new layout
- Input Gain -> INPUT TRIM
- Delay Time -> TIME
- Sync + Sync Division -> SYNC
- Feedback -> FEEDBACK
- Mix -> MIX
- Low Cut -> LOW CUT
- High Cut -> HIGH CUT
- Modulation Depth -> DEPTH
- Modulation Rate -> RATE
- Drive -> DRIVE
- Vintage parameter -> EVOLVE UI position (temporary semantic mapping; DSP parameter ID remains `vintage`)
- Output Gain -> OUTPUT TRIM

## Figma controls not yet represented by processor parameters
These should be added in a later DSP/APVTS pass rather than faked as functional controls:
- Width
- Drift
- Shape
- Bloom
- Ping-Pong
- Mode
- Quality
- Freeze
- Oversampling
- Bypass
- MIDI state

## Important
The editor is intentionally fixed at 1536 x 1024 for this first fidelity pass. Responsive UI scaling should be added after the 1:1 Figma comparison is approved.

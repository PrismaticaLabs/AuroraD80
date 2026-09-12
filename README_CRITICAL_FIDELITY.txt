AURORA D80 — FIGMA FIDELITY CRITICAL CANDIDATE 1.0.1

Reference
- Figma: Production Candidate v1.4, node 6512:5804
- Master design canvas: 1536 x 1024
- Default plugin editor: 1152 x 768 (75%)
- Free corner resizing: disabled in this fidelity candidate

WHAT CHANGED IN THIS CANDIDATE
- Exact Figma module artwork for chassis, brand header, meters, display,
  Delay Core, Tone/Stereo, Modulation/Character, Utility and Preset bars.
- Production knob rebuilt from the exact applied-knob Figma assets:
  body, tick ring, outer shadow, bezel, pointer glow, pointer and highlight.
- Figma master coordinates used for all knob centres and labels.
- Figma min/mid/max scale markings restored around the production knobs.
- Input/Output Trim are -12..+12 dB controls; DSP converts dB to linear gain.
- Feedback/Mix runtime display is percentage-correct (Deep Horizon = 35% / 35%).
- Central live time uses a fixed dot-matrix renderer inside the approved housing.
- Meter glass/bezel/screws remain from Figma; only the individual LED cells are dynamic.
- Freeze and Bypass use the exact exported Figma SVG glyphs for runtime state.
- Mode, Quality, Oversampling, Sync, Ping-Pong, Shape, Division and A/B state
  overlays preserve the approved Figma housings.
- Legacy JUCE renderer remnants that caused project warnings were removed.

FIRST BUILD ON MAC
1. Double-click REGENERATE_XCODE.command.
   This downloads the exact Figma assets into Resources/ and resaves the .jucer.
2. For a quick app test in Xcode, select AuroraD80 - Standalone Plugin / My Mac.
3. For Logic, double-click BUILD_INSTALL_AU.command.
   It builds and installs ~/Library/Audio/Plug-Ins/Components/AuroraD80.component.

IMPORTANT
- Run REGENERATE_XCODE.command before opening/building this candidate.
- Figma MCP asset URLs are temporary, so fetch them promptly.
- Logic's gray Audio Unit wrapper bars are host UI, not part of AURORA's Figma frame.
- This environment cannot execute Xcode/macOS builds. The candidate passed static
  project/script checks here, but Xcode is the authoritative compile validation.
- Oversampling has UI/state values but is not yet a dedicated real oversampling DSP
  implementation; do not treat this candidate as the commercial release build.

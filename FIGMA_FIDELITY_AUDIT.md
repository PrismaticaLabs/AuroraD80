# AURORA D80 — Figma Fidelity Audit

Target: Figma Production Candidate v1.4 (node 6512:5804)
Master canvas: 1536 x 1024
Default AU size: 1152 x 768 (75%)

## Critical fixes included
- Exact Figma raster exports retained for chassis, header, meters, display, modules, utility and preset bars.
- Applied production knob rebuilt from the exact Figma knob body/tick/bezel/shadow/pointer assets (node 6512:6749).
- Removed the previous hand-drawn JUCE knob approximation and value arc.
- Added Figma min/mid/max scale numbers around all production knobs at master coordinates.
- Input/Output Trim now use -12 dB..+12 dB and display dB values; DSP converts dB to linear gain.
- Feedback and Mix value labels now correctly show 35% instead of 0%.
- Deep Horizon defaults aligned to the approved visual reference (578 ms, 19.0 kHz high cut, 10% Drive, Sync enabled, Output Trim -1.5 dB).
- Central time readout uses a fixed dot-matrix renderer so live values retain the display aesthetic without generic font geometry changes.
- Meter repaint no longer blacks out the full Figma glass/chamber; only the LED cells are dynamically neutralised/lit.
- Quality labels changed to LO FI / STANDARD / HI FI to match the approved utility bar.
- Oversampling choices changed to OFF / 2X / 4X / 8X per the Figma component contract.
- Removed old renderer remnants that generated project warnings: bg1, drawPanel, drawScrew, drawMeter.
- Default size remains fixed at 75% to preserve pixel locking in Logic.

## Build order
1. Run REGENERATE_XCODE.command (downloads current Figma assets and resaves the Projucer project).
2. Build/test Standalone if desired.
3. Run BUILD_INSTALL_AU.command to build and install the AU component.

## Known host-side UI
Logic's top Audio Unit wrapper bar and bottom host strip are outside the AURORA editor and are not part of the Figma design.

## Critical candidate additions (1.0.1)
- Production knob uses the exact applied-knob assets from Figma node 6512:6749 rather than a JUCE approximation.
- Runtime state overlays are restricted to changing pixels/values; exported housings/materials remain the visual authority.
- Freeze/Bypass runtime glyphs use the exported Figma SVGs.
- Meter dynamics only repaint the 20 L/R LED cells, preserving bezel/glass/hardware artwork.
- Static audit: balanced C++ delimiters; .jucer XML parses; shell scripts pass `bash -n`; no old `bg1`, `drawPanel`, `drawScrew`, or `drawMeter` renderer remnants remain.

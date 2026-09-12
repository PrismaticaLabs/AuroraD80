AURORA D80 — FIGMA ALIGNMENT 75
================================

Purpose: internal-slot centering correction against Figma Production Candidate v1.4.

Key correction:
- Production Structure artwork lives at parent offset (-4,+6).
- Applied Knob integration lives at parent offset (+4,-6).
- Previous build ignored both parents, producing an 8 px horizontal and 12 px vertical relative drift.
- This build resolves everything into the Figma root coordinate space before applying the single 75% scale.

Default editor: 1152 x 768
Master design: 1536 x 1024
Resize: fixed in this fidelity build

Run:
1. REGENERATE_XCODE.command
2. BUILD_INSTALL_AU.command

# AURORA D80 — Logic/AU proportional resize fix

Fixed 2026-09-01.

The previous build scaled child controls with independent X/Y factors while `paint()` remained in fixed 1536x1024 coordinates. Logic can provide AU editor bounds whose outer window ratio differs from the design ratio, which caused controls to detach from the painted panels when the editor was resized.

This build uses one uniform scale for the full approved 1536x1024 Figma canvas. Painting and interactive components share the exact same scale and offsets. If the host gives non-3:2 bounds, the UI is centred and letterboxed rather than stretched.

Also scales slider/preset labels, ComboBox fonts, and TextButton fonts with the editor.

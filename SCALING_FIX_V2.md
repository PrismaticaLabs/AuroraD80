# Scaling Fix V2

Root fix for Logic/AU resizing:

- The approved Figma coordinate system remains 1536 x 1024.
- All artwork and all JUCE controls now live inside one fixed DesignSurface.
- Logic resizes the DesignSurface as a single transformed component.
- Child controls are no longer individually scaled/repositioned.
- A 3:2 aspect-ratio constrainer is advertised to hosts that honour it.
- If a host supplies another aspect ratio, the complete surface is centred/letterboxed without internal drift.

This specifically fixes controls leaving their panels, Output Trim entering the display, and lower panels separating during resize.

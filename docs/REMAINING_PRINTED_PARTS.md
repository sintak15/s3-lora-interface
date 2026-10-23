# Remaining printed retainers

Load `hardware/enclosure/enclosure-v4-remaining-parts.stl` to print all five pieces together.
These match the 3.3 mm-post CYD front and the face-screw/open-battery base.
No printed battery inserts are needed for the taped holder area.

| Quantity | Piece | Hardware |
| --- | --- | --- |
| 2 | Heltec clamp bars | Four M3 × 8 mm countersunk screws total |
| 2 | GPS antenna backing-PCB rails | Four M2 × 8 mm countersunk screws total |
| 1 | Speaker retaining frame | Two M2 × 8 mm countersunk screws |

The Heltec bars hold its PCB edges; keep their feet off pins and glass.
The antenna rails retain the patch's backing PCB, not its ceramic tile.
The rail with a notch goes on the coax-exit side (assembly left).
The speaker frame feet contact the housing rim, keeping its diaphragm clear.
Confirm actual rim thickness and fastener engagement before tightening.

On the supplied plate, the two long Heltec bars are at the lower Y end, the
large oval frame is behind them, and the two small antenna rails are to their
right. All are flat in their intended print orientation. Use millimetres,
100% scale, 0.2 mm layers, 3 walls and supports OFF. Do not rotate onto edges.
Print-time and physical retention are not verified by the mesh checks.

Individual exports are also in `hardware/enclosure/`:
`hardware/enclosure/enclosure-v4-heltec-clamps.stl`, `hardware/enclosure/enclosure-v4-gps-rails.stl`,
`hardware/enclosure/enclosure-v4-speaker-frame.stl`.

The lightweight front's GPS receiver itself uses two narrow zip ties rather
than keeper tabs. The power assembly uses two ties and the MAX breakout one.
These straps and the battery-holder tape are not printed components.

Regenerate from `hardware/enclosure/enclosure_v4_remaining_parts.scad`, selecting `retainer_part`.

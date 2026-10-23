# Measured CYD front revision

STL: `hardware/enclosure/enclosure-v4-front-cyd-3p3mm.stl` in `hardware/enclosure/`.
Source: `hardware/enclosure/enclosure_v4_front_cyd33.scad`.

- CYD posts: **3.3 mm above the retained 2.4 mm inside floor**, so the PCB
  seats 5.7 mm from the exterior front. This lowers it by 2.9 mm.
- CYD pilot diameter and 42 × 78 mm mounting pitch are retained.
- CYD USB centre moves from 11.2 to 8.3 mm from the exterior face; the
  18 mm-wide opening starts at Z=2.8 and remains open to the shell seam.
- Heltec mounting height, board XY positions, display openings, shell mating
  rim and case screw positions remain the lightweight V4 dimensions.
- The lightweight floor pockets and two-strap GPS receiver retention remain.

Print face down as supplied, 100% scale, millimetres, supports OFF with normal
short-bridge handling enabled. Start at 0.2 mm layers and 3 walls. The model
has not been physically test printed; check actual screen clearance before
tightening the boards.

**CYD screws:** use M3 × 4 mm with the nominal 1.6 mm PCB. The old M3 × 6 mm
screws are too long for these lowered posts. Pilots have 2.8 mm depth above a
0.5 mm solid floor; with the nominal PCB a 4 mm screw engages about 2.4 mm.
Verify actual PCB thickness and screw length before tightening.

The 3.3 mm post dimension comes from the user's physical measurement. The
earlier assumed glass-to-PCB stack has not been remeasured; component preview
ghosts retain the old conservative envelope and do not certify glass clearance.

## Matching base

Use `hardware/enclosure/enclosure-v4-base-cyd-3p3mm.stl`, generated from
`hardware/enclosure/enclosure_v4_base_cyd33.scad`. It retains the 104 × 124 mm rear shell,
34 mm shell depth plus 1.8 mm alignment lip, and production seats for the
single battery holder, intact power assembly, MAX breakout and speaker.
Case depth remains 50 mm when assembled; case screws remain M3 × 40 mm.
The speaker needs the separate V4 speaker retainer from `build/enclosure-v4/`.

Print at 100% in millimetres, exterior rear face on the bed and seats upward,
as supplied. Start at 0.2 mm layers, 3 walls and 15% infill. Inspect the charging
USB opening, switch opening and SMA hole in the slicer; use local supports
where those horizontal openings need them. Unlike the revised front, this
base has not been redesigned to eliminate all elevated port bridges.

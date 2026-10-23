# Faster V4 base

Use `hardware/enclosure/enclosure-v4-base-fast.stl`, from `hardware/enclosure/enclosure_v4_base_fast.scad`.
It matches `hardware/enclosure/enclosure-v4-front-cyd-3p3mm.stl` with the same 104 × 124 mm
footprint, 35.8 mm print height including lip and 50 mm assembled case depth.

The broad floor is internally pocketed from 2.4 to 1.6 mm. Full original
thickness remains under component features and screw pillars, with a 1 mm
border, and at the wall root. Straight sections of three walls are reduced
internally to 1.8 mm; corners, screw pillars, upper mating rim, top hardware
wall and the charging opening retain their original thickness. All component
seats and retention features, exterior dimensions and port positions remain.
The thinner floor and walls are more flexible and less impact resistant.

Print exterior rear face down, as supplied, at 100% scale in millimetres.
For a 0.4 mm nozzle that supports this layer height, try 0.28 mm layers,
3 walls, 3 top/bottom layers and 10% rectilinear infill. This uses about
29% fewer layers than 0.2 mm, which is not a guaranteed 29% time saving.
Use your established machine speed/flow settings. Inspect the USB, switch
and SMA openings and use local supports if their bridges require them.
Avoid a raft; use a brim only if your normal bed adhesion requires it.

Compare both models with identical slicer settings to isolate geometry
savings. A solid-mesh volume reduction does not equal the slicer's filament
or time reduction, especially when thin areas become completely solid.
The base remains untested physically; check strength and fit before field use.

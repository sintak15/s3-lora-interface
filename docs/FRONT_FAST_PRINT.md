# Lightweight V4 front, face-down print

Use `hardware/enclosure/enclosure-v4-front-fast-no-supports.stl`.
It is the full front shell, not a fit coupon. Import in millimetres at 100%.
Leave the broad exterior face on the bed and all component mounts pointing up.
The footprint is 104 × 124 mm, suitable for a 180 × 180 mm bed.

Suggested start with a 0.4 mm nozzle: 0.2 mm layers, 3 walls, 4 top/bottom
layers, 10–15% rectilinear infill, supports OFF. The two bottom USB slots
are open at the shell seam; they require no spanning roof. Raised strap
anchors have short tunnels (about 3.2 mm bridging span); enable bridge handling.
Inspect the slicer layer preview before printing. This version is designed
for support-free printing but has not been physically test printed.

The broad floor is pocketed internally to 1.6 mm; full 2.4 mm skin remains
under mounts and around the wall root. The exterior face stays flat.
Outer dimensions, shell seam, screw datums, CYD/Heltec display openings and
PCB seats remain unchanged. The GPS cover remains unbroken plastic.
The thinner floor is more flexible than the original. Material-volume savings
are recorded in `hardware/enclosure/enclosure-v4-front-fast-no-supports.json`; slicer filament
and time depend on infill, bridging, speeds and machine settings.

The GPS receiver's overhanging corner keeper tabs have been removed.
Their vertical guides remain, with a second pair of strap anchors at the
other end. Secure the GPS receiver with TWO narrow nylon straps using both
pairs of anchors, routing clear of its components and the antenna feed.
All other front retention parts and screws remain the V4 parts.

Regenerate by rendering `hardware/enclosure/enclosure_v4_front_fast.scad` in OpenSCAD and exporting
as STL. The standard `hardware/enclosure/enclosure_v4.scad` front remains available separately.

# V4 3D printed fit trays

Print `hardware/enclosure/enclosure-v4-fit-front.stl` and
`hardware/enclosure/enclosure-v4-fit-rear.stl` separately, flat as supplied,
at 100% scale in millimetres.
Each footprint is 104 × 124 mm and fits a 180 × 180 mm bed.
Use the same material and slicer tolerances intended for the final case.
Start with 0.2 mm layers and 3 perimeters. Inspect the raised mounts,
tie tunnels and GPS keepers in the slicer and add local support as needed.

These are shallow versions of the actual shells, with 7 mm perimeter walls.
The production floor thickness, screen openings, component seats and internal
mount heights are retained. Portions of corner pillars remain above the wall.
The rear is already flipped into its production print orientation.

With power disconnected, test the CYD hole spacing and screen opening, Heltec
supports, GPS board and antenna backing-PCB supports in the front tray. Test
the intact power assembly, single battery holder, MAX board and speaker in the
rear tray. Do not force boards onto unverified mounts.

Use the existing clamps, antenna retainers and speaker retainer from
`build/enclosure-v4/` to check retention if desired. The existing
`build/enclosure-v4/fit_coupon.stl` provides a small USB/post/screw gauge.

The trays check each half independently. Their shortened walls omit upper
connector openings and the alignment lip; they do not verify closed-case
clearance, cable routing, SMA/switch placement or the power USB opening.

Regenerate using OpenSCAD with `part="fit_front"` or `part="fit_back"` in
`hardware/enclosure/enclosure_v4.scad`.

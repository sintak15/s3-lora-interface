# Antenna shifted toward the right wall for pins-down NEO-6M mounting

Full face: `enclosure-v4-front-gps-pins-down.stl`.
Small first-print test section: `enclosure-v4-gps-pins-down-coupon.stl`.
Source: `enclosure_v4_front_gps_pins_down.scad`.

Right is **+X looking into the face while its exterior rests on the build
plate**, as the user specified. The antenna, its bearing ledges and its four
retainer posts all move **2.5 mm right**. The antenna footprint becomes
X=69.5–94.5, Y=90.965–115.965. Retainer posts are now X=65 and 99, Y=99 and 108.
Their right edges stop at X=101.1, leaving 0.5 mm to the nominal inner wall.
The 29.8 mm inner-face spacing is retained.

The NEO-6M board stays at Z=18 with its same four screw centres. The right
receiver columns gain outboard spines so moving the antenna relief does not
leave thin slivers. Spines stop short of the mating lip and are relieved around
the upper-right case pillar. The upper relief taper changes slightly to join
the bearing faces without cutting into the screw openings.

The existing **GPS repair antenna rails** (`enclosure-v4-antenna-rails-repair.stl`)
can be reused: ledges and posts shift by the same amount, so the individual
rails' local geometry and screw pattern do not change. Use the repair rails,
not the older, shorter rails from before the antenna-post widening.

All other latest changes remain, including 20.1 mm Heltec bracket-post height,
the lowered antenna seat, reinforced mounts and face-entry case screw holes.
Use the latest M2 × 4 mm pan-head NEO-6M screws. The existing MAX screw-post
base remains the intended matching base.

The user physically tested the pins-down arrangement and confirmed that
moving the middle antenna posts resolves clearance; a pin-height measurement
is not required for this revision. An optional small test section is included
to try the exact shifted layout before printing the full face. The new STL
itself has not yet been physically printed. Check screw-head lands when
mounting the board on its opposite side.

A mesh intersection check against the latest MAX screw-post base produced no
solid intersection, excluding 0.05 mm of axial seam contact. This checks the
printed parts, not loose wiring or electronic components.

Print exterior face down, millimetres, 100% scale, 0.2 mm layers and 3 walls.
Inspect the outboard reinforcement's upper taper in the slicer; use local
support if your printer requires it. Physical fit and strength are untested.

# NEO-6M screw-post face

STL: `hardware/enclosure/enclosure-v4-front-neo6m-screw-posts.stl`.
Source: `hardware/enclosure/enclosure_v4_front_neo6m_screws.scad`.

The user confirmed that the board's four holes match the existing four
support centres. Those centres and the Z=18 mm PCB seating height are retained.
Each support now has a **1.6 mm pilot, 6 mm deep**, for an M2 screw. The top
bearing surface broadens to **4.2 mm diameter**, with a printable taper above
the antenna/feed clearance zone. The lower 3 × 3 mm columns stay within the
original support footprints so they do not newly encroach on the patch below.

Use **four M2 × 6 mm pan-head screws** with the nominal 1.6 mm PCB thickness.
They engage about 4.4 mm, leaving 1.6 mm before the pilot bottom. Verify the
actual hole size, PCB thickness and clear screw-head land on the actual board.
Tap M2 pilots if needed and tighten gently; do not use countersunk heads to
wedge into the PCB mounting holes.

GPS-board zip-tie anchors and the old keeper guides/tabs are omitted because
the board is now screw retained. The separate patch antenna's supports,
additional 0.5 mm/side print clearance and retaining rails are unchanged.

The face retains all previous revisions: CYD 3.3 mm posts, Heltec bracket
screw posts lowered 5.2 mm, reinforced Heltec roots and 3.4 mm clearance
holes for face-entry case screws. The existing bracket-fit caveat still
applies: inspect its underside feet before seating it lower.

Print exterior face down, 100% in millimetres. Mesh validation does not verify
physical screw fit or mounting strength. The current base remains compatible.

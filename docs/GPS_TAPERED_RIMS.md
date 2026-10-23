# Support-saving GPS rims and tighter CYD pilots

Full face: `enclosure-v4-front-gps-tapered-rims.stl`.
GPS test section: `enclosure-v4-gps-tapered-rims-coupon.stl`.
CYD screw-fit test: `enclosure-v4-cyd-tight-test.stl`.

The blue-marked undersides now transition at **45 degrees**, instead of having
flat undersides that require supports. The transition begins at Z=11.6, giving
0.3 mm clearance over the complete 25 × 25 × 7.5 mm antenna at Z=3.8–11.3.
The antenna pocket remains 27 × 27 mm through the antenna height. Check the
actual coax feed against the new tapered material in the small test section.

NEO-6M seats remain at Z=18 and all hole centres are unchanged. The M2 pilot
depth is now **2.8 mm**, keeping it enclosed within the tapered top material.
Use M2 × 4 mm pan-head screws with the nominal 1.6 mm board: approximately
2.4 mm engagement and 0.4 mm bottom clearance. These changes retain solid
rims around the screw openings while allowing the underside to slope outward.

CYD pilots are further reduced from 2.0 to **1.8 mm**, as the user found the
previous holes too loose. They still have 2.8 mm blind depth and the posts
remain 3.3 mm above the retained inside floor. The small test post reproduces
that exact geometry. Try it with the actual screws first; this is deliberately
undersized for M2.5 and may need tapping. Do not force M2.5 screws into a pilot
that splits or excessively stresses the post. Nominal M2.5 × 4 mm screws fit
the existing depth with a 1.6 mm PCB; verify actual dimensions.

The latest right-shifted antenna location, lower square ledges, level antenna
brackets, original receiver height, restored Heltec post height, reinforced
roots and front-entry case holes remain. The current antenna brackets and
MAX screw-post base remain compatible.

Print exterior face down, 100% scale, 0.2 mm layers and 4 walls. The revised
GPS cap undersides are designed to print without supports. Tie-anchor tunnel
roofs still use short bridges; verify slicer preview. Printability and screw
retention still depend on material, extrusion and the actual printer.

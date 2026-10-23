# Flat antenna between lowered ledges and level brackets

Full face: `enclosure-v4-front-antenna-ledges-lower.stl`.
Small test: `enclosure-v4-antenna-ledges-coupon.stl`.
Source: `enclosure_v4_front_antenna_ledges_lower.scad`.

The user confirmed the **whole antenna is 7.5 mm thick** and will sit flat on
the square support ledges. The square ledges are therefore lowered by **7.5 mm**,
not by the backing PCB thickness. Heights from the exterior front face:

| Surface | Height |
| --- | --- |
| Full-thickness inside floor under ledges | 2.4 mm |
| Square ledge top / antenna underside | 3.8 mm |
| Antenna top (3.8 + 7.5) | 11.3 mm |
| Round antenna bracket screw-post top | 11.3 mm |

The ledges rise 1.4 mm above the retained inside floor. With the measured
antenna installed, the rail can bear flat on both the antenna top and round
post tops. This is nominal zero clearance; physically verify the assembled
stack before tightening. The modeled air space under the antenna is now
1.4 mm rather than the earlier 3 mm design baseline. No RF performance
comparison has been performed for this revised spacing.

The rightward 2.5 mm antenna shift, 29.8 mm clear post spacing, robust NEO-6M
screw columns at the original 18 mm seat height and restored 20.1 mm Heltec
bracket posts remain. The existing GPS-repair rails can be reused; the
individual rail shape is unchanged. Use the current M2 × 4 mm NEO-6M screws
and M2 × 8 mm countersunk antenna-rail screws.

The MAX screw-post base remains compatible. Print the small test first at
100% scale in millimetres, exterior face down. This lowers the four square
antenna ledges only; it does not lower the round bracket screw posts.

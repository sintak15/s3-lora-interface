# Reinforced zip-tie anchors

Latest base: `enclosure-v4-base-heavy-ties.stl`.
Matching face: `enclosure-v4-front-heavy-ties.stl`.
Sources: `enclosure_v4_base_heavy_ties.scad` and
`enclosure_v4_front_heavy_ties.scad`.

Every printed strap anchor is reinforced: the four power-board anchors,
two MAX anchors in the base, and four GPS receiver anchors in the face.
The battery area stays open for tape; the speaker retains its screw frame.

- Anchor body length increases from 6 to **8 mm** along the tie width axis.
- Body height above its root increases from 5 to **6 mm**.
- Sidewall material beside the tunnel increases from 1.3 to **2.3 mm** each.
- Roof thickness above the tunnel increases by **1 mm**.
- Each root gains a tapered **3.2 × 9 mm foot**, blending into the floor.
- Tunnel clear size remains **3.4 mm wide × 1.6 mm high**. Its centre and
  height remain unchanged. The short tunnel passage length remains 3.2 mm
  through the foot; feed the same narrow nylon ties through it.

The lightweight models retain original 2.4 mm floor thickness under the
enlarged anchor footprints with a 1 mm border. The existing inside-floor
pockets stop short of these footprints. Exterior faces remain flat on the bed.

The base retains its 48 × 80 mm flat battery area, charging USB at the lower
end of the side wall, relocated MAX and face-entry M3 × 25 mm case screws.
The face retains CYD posts 3.3 mm above the floor and its revised USB opening.
This new face has 3.4 mm through-clearance case holes for face-entry screws;
no clearance drilling is required on this new print. The previously printed
face still needs its case pilots drilled through if reused.
The existing Heltec clamp bars, individual antenna rails and speaker frame
remain compatible.

The reinforced face also enlarges all four Heltec clamp posts from 5.2 to
7.2 mm diameter with tapered 9 mm roots. Their CYD-facing side is flattened
to preserve clearance. Screw centres, pilots and clamp seating height remain
unchanged, so the existing clamp bars fit. Do not overtighten the screws.
The GPS antenna stays 25 × 25 × 7.5 mm. The surrounding posts and ledges
move outward 0.5 mm per side; the carrier bearing overlap is reduced from
1.2 to 0.7 mm per side. The two antenna rails sit farther apart, but each
separate rail's own screw pattern is unchanged. The optional fresh export is
`enclosure-v4-gps-rails-clearance.stl`; existing separate rails can be reused.
The ceramic-to-front air gap and support height are unchanged. Check that
the actual backing PCB can safely bear on the 0.7 mm ledges before tightening;
retention must not load the ceramic. No physical fit test has been performed.

Print 100% scale in millimetres, exterior face down. Use 3 or more walls,
0.2–0.28 mm layers and your normal short-bridge settings. Inspect the base
side openings for local support needs. The strengthened geometry has not
been physically load tested; tighten ties until secure without crushing parts.

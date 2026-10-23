# MAX module screw-post base

STL: `hardware/enclosure/enclosure-v4-base-max-screw-posts.stl`.
Source: `hardware/enclosure/enclosure_v4_base_max_screws.scad`.

The user confirmed that the module holes match the four existing support
centres. Those centres are retained. The PCB seat stays **4.4 mm from the
exterior rear face**, and the four supports become **4.2 mm-diameter posts**.
Each has a **1.6 mm pilot, 3.2 mm deep**. The pilot bottom is 1.2 mm above
the exterior, leaving the rear surface closed. Full original floor thickness
is retained around the post roots by the lightweight floor's keepouts.

Use **four M2 × 4 mm pan-head screws**, assuming a 1.6 mm PCB. These engage
about 2.4 mm, leaving 0.8 mm before the blind pilot bottom. Check actual PCB
thickness and screw-head land before tightening. Longer screws can bottom out
or break through the rear. Tap the pilots if needed and tighten gently.

The MAX tie anchors are removed; its locating fences remain. The power board
still uses the strengthened tie anchors. All latest base features remain:
48 × 80 mm open taped-holder area, charging USB near the bottom wall,
speaker mount and blind posts for face-entry M3 × 25 mm case screws.
The NEO-6M screw-post face and existing speaker frame remain compatible.

Print exterior rear face down, 100% in millimetres. Use 0.2–0.28 mm layers,
3 walls, and inspect side-port bridges for local support needs. Geometry
checks do not replace physically checking component and screw fit.

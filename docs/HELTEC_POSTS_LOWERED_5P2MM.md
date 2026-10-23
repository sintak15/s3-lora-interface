# Heltec bracket screw posts lowered by 5.2 mm

STL: `enclosure-v4-front-posts-lowered-5p2mm.stl`.
Source: `enclosure_v4_front_posts_lower52.scad`.

The four reinforced Heltec bracket screw posts now end at **Z=14.9 mm**
instead of 20.1 mm, measured from the exterior front face. Their diameter,
tapered roots, screw centres and 2.5 mm pilot diameter are unchanged.
The blind pilots move down with the posts and remain 8 mm deep.

The Heltec PCB support plane remains **Z=8.2 mm**. CYD posts remain 3.3 mm
above the inside floor. Reinforced ties, extra GPS antenna clearance and the
3.4 mm face-entry case-screw holes are retained. The latest base still fits.

This export changes the posts only; it does not change the separate brackets.
If the existing brackets descend by 5.2 mm, their underside contact feet also
descend by 5.2 mm. The earlier CAD bracket tips were at Z=9.8 mm; unchanged
brackets seated lower would put those tips at Z=4.6 mm. This conflicts with
the old nominal PCB contact plane, so physically check the actual brackets,
PCB and pin locations before tightening. The user's measured overall
screen-to-pin height is 15.20 mm, but the individual LCD-to-PCB stack is not
confirmed. Do not force brackets down onto glass, pins or the PCB.

Print exterior face down at 100% scale in millimetres. This post-only
revision is not physically tested; it implements the user's specified drop.

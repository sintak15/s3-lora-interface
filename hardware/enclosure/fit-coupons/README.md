# Quick V4 fit coupons

**Heltec screen check:** `lightweight/all-coupons-screen-fit-180mm.stl`
replaces the single Heltec station with an `OLED FIT` bezel fixture and includes
two matching clamps (eight printed pieces total). It fits the 180 mm bed.
For just the new fixture, use `lightweight/labeled/heltec_screen.stl` with
two copies of `lightweight/labeled/heltec_clamp.stl`.

The bezel uses the actual 19.2 × 27.9 mm opening, 1 mm corner radius,
2.4 mm front skin, PCB support height and both production clamp stations.
With the front face flat down, place the Heltec screen facing the bed, onto
the narrow PCB-edge seats. The board ends should sit 1 mm inside the fixture's
straight end edges; centre the PCB across the edge supports. The clamps do not
positively locate the board along its length, so check this alignment before
tightening. Fit both clamps gently using four M3 × 8 mm countersunk screws.
Remove the name tags first. Lift the fixture and check that the full active
display is visible through the opening, centred, and that the glass is clear
of the bezel without pressure or bowing. This verifies the intended placement;
it does not add longitudinal stops to the final enclosure.

**Lower-filament version:** `lightweight/all-coupons-labeled-180mm.stl` retains
all seven named checks. The CYD mounting frame has 3 mm wide, 0.8 mm thick
connecting rails; the USB coupon keeps the wall and narrow mounting fingers.
Rear seat samples now test only the first 6 mm of each component with open
centres; the Heltec station has a narrower base and labels have thinner backing.
Contact heights, hole pitch and diameters remain the production dimensions.
These samples are more flexible: support them while fitting components and
avoid tightening screws hard. Print at 0.2 mm layers, 100% scale, as supplied.
Slicer filament totals include supports and may differ from solid-model savings.

**Labeled plate:** `all-coupons-labeled-180mm.stl` adds raised name tags:
CYD HOLES, CYD USB, HELTEC, CLAMP, POWER, BATTERY and MAX.
Each tag connects with a 0.6 mm thick tab; clip the tab before fitting parts.
The plate is 124.6 × 137.8 mm and fits the same 180 mm bed at 100% scale.
Individual labeled STLs are in `labeled/`. Regenerate tags with
`python tools/label_fit_coupons.py`, then assemble with
`python tools/layout_fit_coupons.py --labeled`.

For one print, load `all-coupons-180mm.stl`. It includes all seven coupons
flat on the bed, with at least 3 mm between their bounding boxes and 7 mm bed
margin. The layout measures 124.6 × 92.8 × 20.1 mm. Keep 100% scale and the
supplied orientation; no automatic arrangement is needed. Inspect supports for
the same raised features described below. Regenerate with
`python tools/layout_fit_coupons.py`.

Print only what you need, at **100% scale in millimetres**, flat as supplied.
Start with `cyd_mounts.stl` and `heltec.stl`. These remove the broad tray floors
and walls. Print time depends on your slicer; no time estimate is guaranteed.

| STL | What to check |
| --- | --- |
| `cyd_mounts.stl` | All four CYD holes, 42 × 78 mm pitch, actual post height and blind pilots; thin connecting rails preserve spacing. No screen bezel or USB wall. |
| `cyd_usb.stl` | CYD lower pair of posts and actual bottom-wall USB opening; insert your real cable. Does not check the full four-hole pattern. |
| `heltec.stl` | One full-height clamp station: PCB edge seating, clearance around soldered pins and screw posts. Supports only a narrow section of the board. |
| `heltec_clamp.stl` | One production clamp, to use with the Heltec station and two M3 × 8 mm countersunk screws. |
| `power.stl` | First 10 mm of the power-board seat and end fences: check width and underside seating. |
| `battery.stl` | First 10 mm of the battery-holder cradle and end fences: check width and underside seating. |
| `max.stl` | First 10 mm of the MAX board seat and end fences: check width and underside seating. |

Use the final case material and normal extrusion/XY compensation so that fit
transfers to the case. Suggested start: 0.2 mm layers, 2 walls, 10–15% infill.
Inspect raised supports and clamp-pad overhangs in the slicer; add local support
as needed. Keep power disconnected and do not force parts into mounts.

The rear coupons deliberately do **not** check full component length, straps,
connector locations or closed-case clearance. The Heltec station checks one
cross-section, not the whole board. No coupon checks the combined case layout.

Open `hardware/enclosure/enclosure_v4_coupons.scad` and choose `coupon`, or supply
`coupon="NAME"` as an OpenSCAD `-D` definition.

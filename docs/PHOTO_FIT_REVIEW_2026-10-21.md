# Front fit review — 2026-10-21

Reviewed all five JPGs from the user's `Photos-1-001.zip`.
Images are observations; the user's requested changes are to reduce the
USB-end gap and bring both devices toward the front face, with a larger
adjustment for the CYD than for the Heltec.

## Findings

- `230859137`: CYD USB-end side view shows substantial open space below the
  PCB/connector in the wall notch. Shortening CYD supports will move its port
  toward the front face, so its cutout must be updated with the support height.
- `230907182`: Heltec USB-end view likewise shows excess space below the port.
  A smaller support-height reduction is requested for this board.
- `230941905`: CYD front view shows a recessed screen and exposed light-coloured
  display frame along one side. Depth and lateral alignment/aperture coverage
  need separate checks; lowering alone may not hide the frame.
- `231107645`: Heltec front view shows the OLED recessed with its metal frame
  visible. Check the powered active area after lowering, so the bezel does not
  obscure pixels or contact the glass.
- `230853593`: interior overview shows both boards and USB-end setback.
  If reducing board-to-wall distance, keep cable plug-body access and avoid
  corner screw bosses. Move holes/supports and screen openings together in Y.

## Current CAD datums (not measurements from the photos)

| Datum | Current value |
| --- | --- |
| CYD PCB support plane, from exterior front | 8.6 mm |
| CYD assumed glass-to-PCB distance | 5.9 mm |
| CYD modeled glass face, from exterior front | 2.7 mm |
| Heltec PCB support plane, from exterior front | 8.2 mm |
| Heltec assumed OLED-to-PCB distance | 5 mm |
| Heltec modeled OLED face, from exterior front | 3.2 mm |
| Both board USB-end Y positions | 11 mm |
| Nominal inner bottom wall Y | 2.4 mm |
| Nominal board-end-to-inner-wall setback | 8.6 mm |

The larger apparent CYD recess conflicts with the model's nominal 0.3 mm
glass-to-full-thickness-skin gap. Measure the actual PCB seating surface to
glass face before changing the support height; the retained 5.9 mm stack datum
may not describe this exact board. Photos are oblique and have no independent
depth scale, so they do not establish a reliable millimetre adjustment.

## Revision dependencies

1. Reduce CYD post height by a measured amount. Correct its stack datum and
   USB-centre height consistently; recheck blind pilots and screw length.
2. Reduce Heltec seats by a smaller measured amount. Move clamp pads/bridges
   and USB-centre height with it, preserving solder-pin clearance.
3. Reduce the USB-end setback if desired by moving each board and its bezel
   opening toward the bottom wall. Validate corner-boss and cable clearance.
4. Size the USB notches around the actual mating cable, preserving a seam-open
   top if support-free printing is retained. Lowering a connector alone does
   not remove excess notch area above it.
5. Recheck CYD aperture alignment and coverage using a powered display.
   Maintain clearance at both glass faces; do not clamp glass against plastic.

No production dimensions or STLs were changed during this photo review.

## Earlier-face comparison requested by user

The earliest enclosure source available here is `enclosure_v2.scad`.
V2 used a 1.0 mm front floor and 3.0 mm CYD bosses starting on that floor:
the CYD support plane was **4.0 mm from the exterior face**, compared with
**8.6 mm in V3/V4**. This is a 4.6 mm increase toward the rear. Of that,
1.4 mm comes from the thicker floor and 3.2 mm from taller posts above it
(V2: 3.0 mm; V4: 6.2 mm). CYD hole pitch remained 42 × 78 mm.

V2's Heltec retention boss started at floor=1.0 and was 6.0 mm tall, topping
out at **7.0 mm**, versus V4's actual PCB-edge seats at **8.2 mm**. That is a
1.2 mm numerical difference, but the V2 boss is outside the PCB near the end
of the case, not a defined PCB support plane. Its 3.6 mm-high side rails were
also outside the PCB outline. Thus V2 does not establish a confirmed Heltec
PCB seating height; 7.0 mm is only a useful trial reference.

V2 CYD board-to-bottom-inner-wall setback was **2.5 mm**, compared with
**8.6 mm** in V4, a 6.1 mm increase. V2 staggered the Heltec to Y=45 mm, so
its longitudinal placement should not be copied into V4's aligned USB layout.

V2 also cut the glass apertures through the complete front depth; V4 cuts
only through its 2.4 mm skin. V2's CYD opening was 50.4 × 69.6 mm versus
V4's 50.8 × 70.0 mm. V2's much larger Heltec opening, 21.5 × 37.0 mm versus
19.2 × 27.9 mm, can change the visual impression of recess as well.

The user's earlier physical fit is stronger evidence for the exact CYD stack
than V4's retained 5.9 mm glass-to-PCB assumption. At V2's 4.0 mm PCB plane,
that assumption predicts a glass face at -1.9 mm (outside the front), showing
that those two datums cannot both describe a flush installation of this board.
Do not restore the old height without correcting the stack model and checking
the thicker V4 skin against the actual glass housing. A 4.0 mm CYD support
plane and a roughly 7.0 mm Heltec trial plane warrant small fit coupons before
the next complete front is printed. No revised STL was produced by this review.

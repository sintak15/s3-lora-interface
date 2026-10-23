# Base with an open battery-holder area

Use `hardware/enclosure/enclosure-v4-base-open-battery.stl`, from
`hardware/enclosure/enclosure_v4_base_open_battery.scad`. It matches the existing measured-CYD
front and retains face-entry M3 × 25 mm case screws and blind base pilots.

There is a **48 × 80 mm flat, unobstructed floor area** for taping in a single
or double battery holder. The old battery cradle, fences and strap anchors
are omitted. In assembly coordinates this area is X=52–100, Y=10–90.
In the supplied rear-face-down STL it is X=4–52, Y=10–90 (mirrored X).
The floor is uniformly 1.6 mm thick within that rectangle.

The previous single holder, 22.6 × 76.4 mm, fits the area. Two such footprints
side by side occupy 45.2 × 76.4 mm. A one-piece dual holder must have its
actual outside dimensions, terminals and wire exits checked against the area.
Keep the holder around **20.9 mm tall or less**, with up to **1 mm tape**;
these are the envelope dimensions checked against the face-mounted components.
Loose wires and connector bends require a physical clearance check.

The MAX breakout moves from X=45.3, Y=45 to **X=70, Y=94**, toward the antenna
end. Its seats, fences and single-strap anchors are retained. The intact power
assembly and speaker retain their existing positions and retention parts.
The speaker retaining frame remains a separate print.

The case footprint is 104 × 124 mm, print height 35.8 mm including lip, and
assembled depth 50 mm. Print exterior rear face down at 100% scale. For a
0.4 mm nozzle, start with 0.28 mm layers, 3 walls, 3 top/bottom layers and
10% rectilinear infill. Inspect side openings for local support needs.

The mesh, flat battery-floor area, component envelope separation and blind
pilot dimensions are checked; the design has not been physically assembled.

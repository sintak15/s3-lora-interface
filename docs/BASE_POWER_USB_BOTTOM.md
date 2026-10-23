# Charging USB at the bottom-wall end

Use `hardware/enclosure/enclosure-v4-base-power-usb-bottom.stl`. This supersedes the open-battery
base when the power assembly's charging connector is at the end nearest the
CYD/Heltec USB wall. Source: `hardware/enclosure/enclosure_v4_base_power_usb_bottom.scad`.

The opening stays on the same side wall. Its centre moves from assembly Y=65
to **Y=27 mm**, measured from the outer bottom wall, using the existing
**16 mm connector-centre offset from the power-board end**. That offset and
the existing 9.5 mm height from the rear exterior are still estimates, not
new measurements. Opening size remains 18 × 12 mm. The old opening is closed.
The side-wall reinforcement follows the new opening. Power-board strap
positions mirror along its length to Y=14 and Y=72 mm.

This is an end-for-end change of the connector position, not an instruction
to rotate the physical board 180 degrees flat: that rotation would also swap
its left/right sides. Seat the real assembly with its USB at the lower end
facing the existing side wall, and confirm the port against the actual cable.

The 48 × 80 mm clear battery area, relocated MAX, speaker mount, matching lip,
and four blind pilots for face-entry M3 × 25 mm case screws are retained.
Print rear face down, 100% scale, in millimetres. Inspect side-port bridges
for local support needs. Actual component/cable fit is not physically verified.

# GPS fit and post-strength revision from the new photographs

Full face: `enclosure-v4-front-gps-repair.stl`.
Matching antenna rails: `enclosure-v4-antenna-rails-repair.stl`.
Small first-print test: `enclosure-v4-gps-repair-coupon.stl`.

- Heltec bracket screw-post tops are restored to **20.1 mm** from the exterior
  face. PCB support height and LCD position are unchanged.
- NEO-6M PCB seats remain **18 mm** from the exterior face; the board is NOT
  raised. Its confirmed screw centres stay unchanged.
- NEO-6M supports use **6 mm bodies with tapered 8 mm roots**, flattened or
  relieved where needed to preserve the adjacent screen and antenna spaces.
- The **7.5 mm antenna height is treated as the complete assembly**, including
  the assumed 1.6 mm backing PCB, instead of adding that thickness again.
  Its backing-PCB seat moves from 12.9 to **11.3 mm**, lowering the antenna by
  1.6 mm while retaining the modeled 3 mm air gap over the ceramic.
- Antenna retaining-post centres move to X=62.5 and 96.5. Their opposing
  surfaces are **29.8 mm apart**. The bearing ledges stay at the existing
  0.5 mm outward adjustment to retain backing-PCB contact.
- A **27 × 27 mm antenna/feed relief** is removed from nearby NEO-6M towers
  up to Z=14.9, then transitions at 45 degrees into the top bearing faces.
  The right receiver towers are therefore asymmetric, leaving room for the
  antenna while retaining wider material on the outside.

**Use four M2 × 4 mm pan-head screws for the NEO-6M in this revision**, assuming
a 1.6 mm PCB. Pilots are 1.6 mm diameter, with a 3.2 mm nominal depth; the
antenna-side bottoms partially open into the relief. Do not reuse longer
screws without checking their ends against the antenna and its feed.
The widened antenna layout requires the NEW matching rail print, using the
existing four M2 × 8 mm countersunk antenna screws. The rails' contact bridges
are wider to reach their posts. The original Heltec brackets remain applicable.

Print the **47 × 40 mm GPS test section first** and dry-fit the actual antenna
and board, with no force. It preserves the exact final mounting geometry.
The photographs do not supply a scale for the complete antenna frame/connector;
clearance is therefore a revised design to test, not a physical-fit guarantee.
Check the backing PCB bears safely on its ledges without loading ceramic.
Confirm the 1.6 mm PCB-thickness assumption against the actual antenna.

Print exterior face down, 100% in millimetres, 0.2 mm layers and 3 walls.
The existing MAX screw-post base, open battery area and front-entry M3 × 25 mm
case screws remain compatible. Preserve room for wires above the receiver.

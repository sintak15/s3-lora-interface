# Enclosure files

All OpenSCAD sources, published STL exports, JSON geometry reports and fit
coupons live in this folder. Run tool commands from the repository root. Part
names and geometry are unchanged by the repository reorganization.

## Current print set

| Part | Printable file | Notes |
| --- | --- | --- |
| Front face | [Tapered GPS supports](enclosure-v4-front-gps-tapered-rims.stl) | [GPS supports and 1.8 mm CYD pilots](../../docs/GPS_TAPERED_RIMS.md) |
| Base and two Heltec brackets | [Combined 180 mm plate](enclosure-v4-latest-base-and-heltec-brackets.stl) | [Base screw posts](../../docs/MAX_SCREW_POSTS.md) |
| Base alone | [MAX screw-post base](enclosure-v4-base-max-screw-posts.stl) | Same base as the combined plate |
| Antenna retainers | [Matching brackets](enclosure-v4-antenna-rails-repair.stl) | [GPS repair and matching rails](../../docs/GPS_PHOTO_REPAIR.md) |
| Speaker retainer | [Frame](enclosure-v4-speaker-frame.stl) | [Remaining printed parts](../../docs/REMAINING_PRINTED_PARTS.md) |

Print at 100% scale in millimetres with the supplied bed orientation. Geometry
checks do not establish physical fit; use [fit coupons](fit-coupons/README.md)
before committing to a full print. See the [V4 assembly guide](../../docs/ENCLOSURE_V4.md).

## Sources and earlier fit variants

The base parametric model is [enclosure_v4.scad](enclosure_v4.scad). Wrapper
sources select each published fit revision; matching JSON files record geometry
checks. Earlier variants are retained for existing prints and fit investigations:

- [V3 prototype](../../docs/ENCLOSURE.md) and [V2 source](enclosure_v2.scad)
- [Faster-print front](../../docs/FRONT_FAST_PRINT.md) and [base](../../docs/BASE_FAST_PRINT.md)
- [3.3 mm CYD seats](../../docs/FRONT_CYD_3P3MM.md) and [M2.5 pilots](../../docs/CYD_M25_PILOTS.md)
- [Reinforced tie anchors](../../docs/REINFORCED_TIE_ANCHORS.md)
- [Face-entry case screws](../../docs/BASE_FACE_SCREWS.md)
- [Open battery area](../../docs/BASE_OPEN_BATTERY.md) and [bottom power USB opening](../../docs/BASE_POWER_USB_BOTTOM.md)
- [Flat antenna ledges](../../docs/ANTENNA_FLAT_LEDGES.md)
- [GPS pins-down fit](../../docs/GPS_PINS_DOWN.md), [solid rims](../../docs/GPS_SOLID_RIMS.md), and [tapered rims](../../docs/GPS_TAPERED_RIMS.md)
- [NEO-6M screw posts](../../docs/NEO6M_SCREW_POSTS.md) and [lowered Heltec posts](../../docs/HELTEC_POSTS_LOWERED_5P2MM.md)

The [export helper](../../tools/export_enclosure.ps1) reads sources from this
folder and writes new renders under ignored `build/enclosure-v4/` by default.
Fit-coupon and plate-layout tools use this folder for their published inputs.

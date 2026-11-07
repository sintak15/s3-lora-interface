# Sensor enclosure prototype and V4 retrofit

These parts target the current V4 tapered-GPS front and MAX screw-post base. Keep your original STL files. The new shells use direct mounts; the retrofit plates are independent alternatives for testing the existing print.

**Print small parts first. Incoming receiver, antenna, ADS1115 and AM2302 dimensions have not been measured. These are fit prototypes, not confirmed production parts.**

## Try the existing enclosure first

| Part | File | Installation |
| --- | --- | --- |
| ATGM336H receiver adapter | [GPS plate](../hardware/enclosure/sensors-retrofit_gps.stl) | Remove old GPS; use its four screw towers and retain the new board with a narrow nylon tie through the slots. |
| ADS1115 adapter | [ADS plate](../hardware/enclosure/sensors-retrofit_ads.stl) | Remove MAX17048; use its four screw posts. Insulate and tie the ADS board to the plate. |
| Smaller antenna carrier | [Antenna insert](../hardware/enclosure/sensors-retrofit_antenna.stl) | Trial-fit in the old 25 mm patch bay. Support carrier edges only; do not clamp or load the ceramic. |
| External keyboard support | [CardKB tray](../hardware/enclosure/sensors-keyboard_tray.stl) | Removable standalone tray, retained with soft ties; connector edge stays open. |
| Cable samples | [Fit coupon](../hardware/enclosure/sensors-fit_coupon.stl) | Check cable/plug clearance before making holes. |

1. Disconnect USB and battery. Remove the failed MAX and old GPS assembly.
2. Print GPS and ADS adapters at 100% in mm, flat face on bed, without supports. Reuse screws only if they engage without bottoming out; the plates add 2 mm thickness. Do not force screws or drill deeper into the shell.
3. Trial-fit the actual boards, including solder joints and cables. The old GPS pilot pattern is 21.73 x 32.38 mm; the MAX pattern is 15.4 x 20.45 mm. A differently revised printed shell needs its own measured pattern.
4. Add insulating spacers where pins project below a board. Avoid compressing components with ties. Check the antenna feed and coax bend without crushing either.
5. Close the shell gently with power disconnected. Check clearance to the battery, ADS, Heltec and brackets. Adapter stack heights and actual module pins are not yet validated; stop if the halves resist closing.
6. Test sensors with the enclosure open before drilling. The existing print has no new TEMT optical opening, AM2302 mounting bosses or keyboard port. For an initial trial, keep these accessories external and route loose wires through an available opening without trapping them at the seam. Permanent holes require measuring and drilling the existing shell.

The antenna insert assumes an 18 mm carrier with a clear perimeter. If the supplied antenna lacks that edge clearance, do not use the insert: adjust the parameters or temporarily secure the carrier by its edges. The retrofit plates do not reproduce the new shell's complete sensor layout.

## Revised direct-mount shells

| Part | File |
| --- | --- |
| Front | [Sensor front](../hardware/enclosure/sensors-front.stl) |
| Base | [Sensor base](../hardware/enclosure/sensors-back.stl) |
| New antenna retainer | [Retainer](../hardware/enclosure/sensors-gps_retainer.stl) |
| External AM2302 holder | [Holder](../hardware/enclosure/sensors-am2302_holder.stl) |

Retains the 104 x 124 x 50 mm case body, screen, Heltec, power, battery, switch and antenna connector locations. The external AM2302 holder protrudes beyond the case. Reuse the existing Heltec brackets and speaker retainer listed in the [enclosure index](../hardware/enclosure/README.md).

The front directly supports the ATGM receiver, antenna carrier and TEMT6000; a 4 mm aperture faces the TEMT phototransistor. The base directly supports ADS1115 in place of MAX. Board supports use ties rather than unverified PCB hole patterns. Thread the receiver tie through both side-rail tunnels beneath the PCB and over its top, clear of components.

The left base wall has a 12 x 8 mm keyboard cable opening centred at Y=100, Z=30 mm. It is a cable opening, not a panel-mount Grove socket or strain relief. Secure the cable internally; verify plug passage with the coupon. CardKB remains a detachable external keyboard.

The right base wall has a 5 mm sensor cable hole and two external blind screw bosses at Y=25/77, Z=35 mm. The separate AM2302 holder attaches here with two M2 screws selected for actual stack thickness and blind engagement. Use soft ties through the holder slots. Keep vents exposed; the small visor is not weatherproof. Mount away from direct sun, rain and internal warm airflow. The holder assumes a 40 x 23 x 14 mm wired housing; measure yours first.

Prototype envelopes in the [parametric source](../hardware/enclosure/enclosure_v5_sensors.scad): receiver 18 x 18 x 8 mm, antenna ceramic 18 x 18 x 6 mm plus 1.6 mm carrier, ADS 18 x 28 x 10 mm, TEMT board 14 x 14 x 7 mm, CardKB 88 x 54 x 5 mm. Adjust dimensions, positions and optical alignment before a full print. Shells have 2.4 mm skins; allow for the thicker floor compared with lightweight V4 variants.

## Wiring and verification

Follow [hardware wiring](HARDWARE_UPGRADE.md), [battery/ADS wiring](BATTERY_STATS.md) and [external AM2302 wiring](AM2302.md). No charging detection or MAX unit is included. CardKB electrical integration remains pending: its 5 V supply does not establish safe I2C voltage compatibility with the CYD. Leave it disconnected until that interface is resolved; no level shifter is assumed installed.

Export reproducibly from the repository root:

```powershell
./tools/export_sensor_enclosure.ps1
```

Outputs default to ignored build/enclosure-sensors. The helper uses OpenSCAD hard warnings and checks closed, consistently wound STL edges. The published meshes passed these checks. Nominal component envelope assertions pass; this does not validate actual hardware dimensions, cable routing, screw length, print tolerances or physical fit.

[Exploded preview](../hardware/enclosure/sensors-exploded.png) / [assembled preview](../hardware/enclosure/sensors-assembled.png).

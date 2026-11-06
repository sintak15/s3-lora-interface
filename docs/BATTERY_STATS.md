# ADS1115 battery voltage monitoring

This branch uses an external ADS1115, not a fuel gauge. It requires no CYD
component changes. Leave R14/R15 and GPIO9 untouched. Remove the failed MAX17048
and its battery/I2C wiring. Keep the existing TP4056 charger and MT3608 boost.
There is no charging detection or remaining-runtime estimate.

## Wiring (all power disconnected)

Use a single standard 4.2 V maximum Li-ion cell. Connect the ADS1115 to the
CYD I2C connector by SIGNAL LABEL, not cable color or connector orientation:

| ADS1115 | CYD / connection |
| --- | --- |
| VDD | CYD 3V3, never 5 V |
| GND | Common protected/system ground |
| SDA | GPIO16 (shared touch/codec bus) |
| SCL | GPIO15 (shared touch/codec bus) |
| ADDR | GND, selecting 0x48 |
| A0 | Divider midpoint below |
| ALERT/RDY | Leave disconnected |
| A1 | Midpoint of a SECOND 10k/10k divider on boosted 5 V |
| A2 | TEMT6000 analog output (sensor powered from CYD 3V3) |
| A3 | Unused; may connect to GND |

The existing touch and codec use 0x38 and 0x18, so 0x48 does not conflict.
Confirm the actual module is ADS1115, not ADS1015, and that any onboard SDA/SCL
pullups go to its 3.3 V VDD.

```text
Switched, unboosted cell positive (AFTER switch, BEFORE boost)
              |
           10 kohm, 1%
              |
              +---------------- ADS1115 A0
              |                    |
           10 kohm, 1%           100 nF ceramic
              |                    |
System GND ---+--------------------+
```

Use two 10 kohm 1% resistors, ordinary 1/4 W is sufficient. The 100 nF capacitor
filters A0 to GND. Add a 100 nF VDD-to-GND decoupling capacitor near the module
if its breakout does not already include one. Divider current at 4.2 V is
approximately 210 microamps while switched on.

Use the switched CELL rail, not boosted 5 V and not an always-live battery tap.
The ADS1115 is powered by CYD 3V3; a live analog input with VDD off can backfeed
through input protection. Power-off behavior must be checked on the assembly.
Do not bridge battery protection by joining raw cell negative to system ground.
Keep CYD's BAT connector empty to avoid its onboard charger/power path.
No transistors, status diodes, charger status taps or CYD solder modifications
are required. Leave existing power/charging connections otherwise unchanged.

## Meter checks before connecting A0

1. Remove battery and all USB cables while assembling the divider.
2. Check resistor values, ground continuity and absence of solder bridges.
3. Power the assembly; measure cell voltage at the switched source and midpoint
   voltage relative to system GND. Midpoint must be HALF the cell voltage:
   4.0 V cell -> 2.0 V midpoint; 4.2 V -> 2.1 V.
4. Verify ADS1115 VDD is about 3.3 V, and SDA/SCL are never pulled to 5 V.
5. With main switch off, check midpoint and ADS1115 supply fall near 0 V.
   Also test charger-USB and CYD-USB combinations: A0 must never stay powered
   when ADS1115 VDD is off. Change switch/sense placement if that occurs.
6. Disconnect power before connecting A0 and reassembling the device.

Never connect the raw cell directly to A0. ADC full-scale settings do not
permit an input above VDD. Probe I2C map should show 0x48 (ADS1115).

## Firmware and displayed data

The driver scans A0, A1 and A2 sequentially once per second, using
+/-4.096 V gain (125 microvolts/count), 128 SPS, comparator disabled. Conversion
completion is polled without delaying the loop. Wrong configuration, bus errors,
short reads, negative/clipped values and conversions that fail to complete
within 100 ms invalidate the reading. The next second retries automatically.

Voltage is filtered; percentage is an approximate interpolated VOLTAGE estimate,
shown with ~. Load, temperature, charging and age affect accuracy. It is not
measured capacity. No charging state is inferred from voltage changes. Low and
critical alerts require 30 seconds at/below 3.65 and 3.50 V; they do not replace
hardware cell protection. Plausible cell range is 2.5-4.35 V.

Defaults in constants.h can be overridden in config/device_config.local.h:

```cpp
#define BATTERY_ADS1115_ADDR 0x48
#define BATTERY_DIVIDER_TOP_OHMS 10000UL
#define BATTERY_DIVIDER_BOTTOM_OHMS 10000UL
#define BATTERY_ADC_SCALE 1.0f
```

After readings settle, compare against the meter. Calibration scale is current
scale multiplied by meter voltage / reported voltage. Don't calibrate against
the boosted output. Missing hardware should show unavailable, never 100% or 5 V.

## Boosted supply monitoring (A1)

Add a SECOND pair of 10 kohm 1% resistors, independent of the battery divider:

```text
Boost converter 5V output --- 10k ---+--- ADS1115 A1
                                   |
                                  10k
                                   |
                               System GND
```

Add 100 nF from A1 to GND. At 5.0 V the midpoint must be 2.5 V. Keep A1 within
0-VDD; do not connect 5 V directly to an ADC input. This reports the actual rail,
including 0 V when it is off and the CYD is USB-powered. It does not measure
charger presence. Measure A1 before connecting it, and check power-off combinations
as for A0: boost output must not remain live when ADS1115 VDD is off.
BOOST_DIVIDER_TOP_OHMS and BOOST_DIVIDER_BOTTOM_OHMS default to 10000UL;
BOOST_ADC_SCALE defaults to 1.0f. Calibrate this separately against a meter on
the boosted output. Firmware accepts 0-6.5 V reconstructed rail voltage; this
plausibility range does not permit excessive analog input voltage.

## TEMT6000 ambient light (A2)

For a three-pin TEMT6000 ANALOG BREAKOUT (check its own labels):

| Sensor | Connect to |
| --- | --- |
| VCC / + | CYD 3V3, NOT 5 V |
| GND / - | System GND |
| SIG / S / OUT | ADS1115 A2 |

A typical breakout already includes the emitter load resistor. For a BARE
TEMT6000 phototransistor, connect collector to 3V3, emitter to A2, and add a
10 kohm resistor from emitter/A2 to GND. Verify the exact package pinout first;
do not add another load resistor in parallel to an existing breakout resistor.
A 100 nF VCC/GND capacitor near the sensor is useful if its board lacks one.
Position the optical face behind a clear opening, away from the screen/backlight
and indicator LEDs. Covering it should lower its output; brighter light should
raise it. A meter check must confirm SIG stays between GND and 3V3.

Firmware reports signal volts and 0-100% RELATIVE TO 3.3 V, not calibrated lux
or a linear percentage of perceived brightness. Zero is valid darkness.
Different breakout resistors and sensor variation change the response. A valid
ADC reading cannot prove a TEMT6000 is physically connected: a disconnected,
grounded input can also read zero. Screen brightness remains manual; no automatic
brightness changes are enabled by this addition.

## API and logs

batteryMeasurementAvailable reports voltage validity; batteryEstimated is true.
battery is estimated percent and voltage is cell volts, null when unavailable.
fuelGauge is none; fuelGaugePresent is false. Legacy gauge SOC/register fields
are null. batteryCharging and batteryChargerConnected are null. Runtime remains
unavailable. The web UI omits charging status; diagnostic APIs retain unknown.

boostVoltageAvailable/boostVoltage expose A1 validity and reconstructed rail
volts. ambientLightAvailable, ambientLightVoltage and ambientLightRelativePercent
expose A2. Unavailable values are null, never retained as live measurements.
auxiliaryAdcErrors counts failed/invalid auxiliary samples. Touchscreen System >
Battery and web Health show both sensors. A3 stays free.

Continuous SD diagnostics/downloads/rotation remain. Readable power-health logs
include A1/A2 observations every 10 seconds; legacy CSV columns stay unchanged. Gauge/register
columns remain for compatibility with validity false and unused raw values zero.
soc_percent holds voltage-estimated percent (-1 when unavailable). cell_mv and
voltage variation columns now use ADS1115 observations. Archives are unchanged.
Historical enclosure files may still include unused MAX mounting posts.

## References

- [Vishay TEMT6000 documentation](https://www.vishay.com/en/product/81579/)
- [TI ADS1115 datasheet](https://www.ti.com/lit/ds/symlink/ads1115.pdf)
- [LCDWiki schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf)

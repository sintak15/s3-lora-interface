# Beginner wiring guide

This guide connects an **LCDWiki 2.8-inch ESP32-S3 CYD**, **Heltec V3.2**,
**ATGM336H GPS breakout** and **ADS1115 battery-voltage breakout**.
It uses the existing single-cell TP4056 charger/protection, switch and MT3608
boost assembly. No temperature/humidity sensor, light sensor or 5V monitoring
divider is needed. The boosted 5V supply still powers the two main boards.

## 1. Gather the parts and identify the labels

You need the four boards above, their antennas, the existing power assembly,
two **10k ohm, 1% resistors**, one **100 nF ceramic capacitor** for the battery
signal, insulated hookup wire, a multimeter and suitable connectors/soldering tools.
A second 100 nF capacitor is useful across ADS1115 power if the breakout lacks one.
A capacitor marked **104** is 100 nF. The two divider resistors and ceramic
capacitors have no positive/negative orientation.

- **CYD** means the touchscreen board. It runs this repository's firmware.
- **Heltec** means the separate LoRa board. It runs Meshtastic.
- **GPIO14** means the pin labeled IO14/GPIO14 on the named board. It does
  **not** mean the fourteenth pin along a header. Identical numbers on different
  boards are separate pins.
- **GND** is the shared electrical reference. **3V3** is a 3.3V supply;
  **5V** is a different supply. Never substitute one for the other.
- **TX** sends data; **RX** receives it. Connect a sender to a receiver.
- **SDA/SCL** are the two I2C signal wires used by the ADC.

Find each signal using the board's printed labels/pinout. The CYD I2C connector
provides 3V3, GND, SDA/IO16 and SCL/IO15. Do not infer pin order from wire colors,
photos of another breakout or which way a connector faces.

**Before touching wiring, unplug the cell and every USB cable.** Attach the
Heltec's LoRa antenna before powering the radio. Keep bare wire ends insulated.

## 2. Understand the power path

Keep the existing assembled charger/protection/switch wiring intact. This is
an overview of its function, not a new pinout for an unidentified charger board:

```text
Single Li-ion cell -> existing charger/protection and switch -> MT3608 boost
                                                                |
                                                          regulated 5V
                                                            /       \
                                                        CYD 5V    Heltec 5V

CYD 3V3 ---------------------------------------------- ADS1115 VDD
System GND --------------------------- CYD / Heltec / GPS / ADS1115 GND
```

Use one standard Li-ion cell with a **4.2V maximum**. The battery sense point
used later is the **switched, unboosted cell positive**, after the switch and
before the boost converter. It is not the boost's 5V output.

| Power wire | Goes to |
| --- | --- |
| Boost OUT+ / regulated 5V | CYD 5V and Heltec 5V |
| Protected/system GND | CYD GND, Heltec GND, GPS GND and ADS1115 GND |
| CYD 3V3 | ADS1115 VDD |
| GPS VCC | Supply allowed by your exact GPS breakout; see step 4 |

Use a common ground connection or small ground distribution point. Several
wires may join that point; they need not all fit into one board's GND pin.
Do not bypass battery protection by tying raw cell negative to system GND.
Keep the CYD BAT connector empty. Charge the cell through the separate charger
connection; the CYD USB port does not charge this project's external cell.

If the boost is adjustable, verify **5.0V with a meter before connecting either
main board**. Reuse the working power assembly rather than guessing B+/B-/OUT
connections: charger versions differ, and a charger alone is not necessarily
a protection circuit. Keep USB unplugged during initial battery-powered checks.

## 3. Connect the CYD to the Heltec: three wires

With all power disconnected, make these connections:

| Wire | Start | End |
| --- | --- | --- |
| 1 | Heltec GPIO39 (TX) | CYD GPIO14 (RX) |
| 2 | CYD GPIO21 (TX) | Heltec GPIO38 (RX) |
| 3 | Heltec GND | Common system GND, also connected to CYD GND |

```text
CYD GPIO14 RX <---------- Heltec GPIO39 TX
CYD GPIO21 TX ----------> Heltec GPIO38 RX
CYD GND ---------------- Heltec GND (common system ground)
```

These are 3.3V data signals. Do not connect either data wire to 5V.
In the Heltec Meshtastic Serial module, use **Enabled**, **PROTO / Protobuf**,
**115200 baud**, **RX pin 38**, **TX pin 39**. These are Heltec pin settings.

## 4. Connect the GPS: three wires

| GPS breakout label | Connect to |
| --- | --- |
| TX / TXD | CYD GPIO3 |
| GND | Common system GND |
| VCC | Supply matching the exact breakout's documented input rating |
| RX / RXD | Nothing; leave disconnected |
| PPS | Nothing; leave disconnected |

**Resolve the GPS supply rating before connecting VCC.** Some carrier boards
accept 5V through a regulator; that does not mean the bare ATGM336H accepts 5V.
Use CYD 3V3 only if the breakout specifies a compatible 3.3V input; use boosted
5V only if it explicitly permits 5V. If the input rating is unknown, identify
the breakout first. Its TX signal must be compatible with 3.3V logic.

Connect the supplied GPS antenna. Leave **CYD GPIO2 disconnected**. There is
no GPS wire to the Heltec: the CYD receives NMEA at **9600 baud, 8N1** and can
forward positions over the link from step 3. Do not connect GPS RX to either board.

## 5. Connect the ADS1115 power and I2C wires

Read the labels on the ADC board; verify it is **ADS1115**, not ADS1015.

| ADS1115 label | Connect to | Why |
| --- | --- | --- |
| VDD / VCC | CYD 3V3 | ADC supply; never 5V in this build |
| GND | Common system GND | Shared reference |
| SDA | CYD GPIO16 / SDA | I2C data |
| SCL | CYD GPIO15 / SCL | I2C clock |
| ADDR | GND | Selects address 0x48 |
| A0 | Leave unattached until step 7 | Battery divider signal |
| A1, A2, A3 | GND | Holds unused inputs at a defined voltage |
| ALERT / RDY | Nothing | Not used |

The CYD's touch and audio codec already share SDA/SCL; keep them connected.
Any I2C pullup resistors on the ADC breakout must connect to **3.3V**.
The ADC uses address 0x48; touch uses 0x38 and the codec uses 0x18.
If the breakout lacks power decoupling, connect a 100 nF ceramic capacitor
between VDD and GND close to the ADC.

## 6. Build the battery divider, leaving A0 disconnected

The divider reduces the cell voltage by half so the ADC can read it.
Call the resistors R1 and R2; both are **10k ohm, 1%**.

1. Connect one end of R1 to switched, unboosted cell positive.
2. Join R1's other end to one end of R2. This joint is the **midpoint**.
3. Connect R2's remaining end to system GND.
4. Connect a 100 nF ceramic capacitor from the midpoint to system GND.
5. Leave the wire from the midpoint to ADS1115 A0 disconnected for now.

```text
Switched cell + (after switch, BEFORE boost)
              |
          R1: 10k ohm
              |
              +---------- midpoint --------> A0, AFTER meter checks
              |              |
          R2: 10k ohm       100 nF
              |              |
System GND ---+--------------+
```

The resistors go **end to end**, not side by side across the same two points.
The capacitor goes from the midpoint to GND, not in series with the A0 wire.
Never connect A0 straight to the battery or the boost output. Leave CYD
R14/R15, GPIO9 and the BAT connector untouched.

## 7. Check with a multimeter, then attach A0

Set the meter to DC volts. Put its black probe on **system GND** and use the
red probe on each test point. Prevent the probe tip from bridging nearby pins.

| Test point while switched on | Expected result |
| --- | --- |
| Boost output | About 5.0V |
| ADS1115 VDD | About 3.3V |
| Switched cell positive | Actual cell voltage, up to 4.2V |
| Divider midpoint | Half the switched cell voltage |

For example, **4.0V cell -> 2.0V midpoint**, or **4.2V -> 2.1V**.
If the midpoint is close to full cell voltage, disconnect power and check R2's
ground connection and both resistor values. Do not attach A0 until this is correct.

Switch off: the midpoint and ADC supply must fall near zero. Check the power
combinations you intend to use, including charger USB and either board's USB.
**The midpoint must not remain powered while ADS1115 VDD is off.** If it does,
change the sense/switch placement before proceeding; the input can backfeed the ADC.
An ADC input must stay between GND and its powered VDD.

Unplug the cell and all USB cables again. Now connect the verified midpoint
to **ADS1115 A0**. Inspect for loose strands, exposed joints and reversed supplies.

## 8. Power on and check each function

1. Power the assembled device. Check that the CYD starts and touch works.
2. Confirm the Heltec link connects. If it does not, check the crossed TX/RX
   wires and the Heltec Serial module settings in step 3.
3. Check displayed battery voltage against your meter. Percentage is only an
   approximate voltage estimate; there is no charging detection or runtime estimate.
4. Open **System > GPS**, or use USB command `gps`. Byte and valid NMEA
   sentence counts should increase before a position fix is available.
5. Take the GPS outdoors with a clear sky view. First acquisition can take
   several minutes. Confirm a fresh position on the map.
6. To share that fix, open **System > GPS > Config > Adv > Use CYD GPS**.
   Wait for setup/readback and an accepted update; see [GPS sharing](GPS_SHARING.md).

The firmware still samples grounded A1/A2; 0V / 0% light does not mean those
sensors are installed. To disable polling the absent temperature/humidity sensor,
copy `config/device_config.example.h` to `config/device_config.local.h` if needed
and add `#define AM2302_DATA_PIN -1`. Rebuild/upload using the [build guide](BUILD.md).
Keep `GPS_RECEIVER_UBLOX=0`, the default for this receive-only GPS.

## Quick troubleshooting

| Symptom | Check first |
| --- | --- |
| No CYD power | 5V output, switch, polarity and common ground |
| Heltec not connected | GPIO39 to CYD14; CYD21 to GPIO38; PROTO, 115200, RX38/TX39 |
| Battery reading unavailable | ADS1115 VDD/GND, SDA16/SCL15, ADDR to GND and A0 midpoint |
| Battery voltage about half the real value | Display should reconstruct the cell voltage; check divider configuration/calibration |
| No GPS bytes | GPS supply rating, ground and TX to CYD3; 9600 baud |
| GPS bytes but no fix | Valid NMEA counts, antenna connection and outdoor sky view |
| Touch stops working after ADC installation | Disconnect power; check SDA/SCL for shorts or swapped wires and 3.3V pullups |

The screen, touch, MicroSD slot and speaker port are already wired on the CYD.
Use the built-in SD slot and dedicated speaker connector; do not add GPIO wires
for them. The [battery reference](BATTERY_STATS.md) includes optional expanded
sensor wiring; its A1/A2 and outside-sensor additions are not part of this build.

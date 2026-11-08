# Wiring guide: ATGM336H GPS and ADS1115 battery sensing

This is the core build for the LCDWiki 2.8-inch ESP32-S3 CYD and Heltec
V3.2 running Meshtastic. It uses an ATGM336H GPS and ADS1115 battery ADC.
The NEO-6M and MAX17048 are no longer used. No DHT22/AM2302, light sensor,
or boosted-5V monitoring divider is installed. The 5V rail still powers the boards.
GPIO numbers below belong to the named board, not physical header positions.

## Power

Disconnect the cell and all USB cables before assembling or changing wiring.
Keep the existing single-cell TP4056 charger, protection, switch and MT3608
boost power path. Use one standard Li-ion cell with a 4.2 V maximum.

- Boosted 5V output supplies CYD 5V and Heltec 5V.
- All modules share protected/system GND. Do not bypass cell protection by
  joining raw cell negative to system GND.
- Keep the CYD BAT socket empty; charge through the separate charger connection.
- Power the GPS according to the actual breakout's input rating. Use 5V only
  if its regulator/input specification permits it; do not assume a bare
  ATGM336H accepts 5V. UART and I2C signals must be 3.3V compatible.

## CYD to Heltec serial

| CYD | Heltec | Purpose |
| --- | --- | --- |
| GPIO14 RX | GPIO39 TX | Heltec to CYD |
| GPIO21 TX | GPIO38 RX | CYD to Heltec |
| GND | GND | Common ground |

Enable the Heltec Serial module in **PROTO / Protobuf** mode at **115200 baud**,
with **RX 38 / TX 39**. Heltec runs Meshtastic; this repository's firmware runs
on the CYD.

## ATGM336H GPS

| GPS | Connection |
| --- | --- |
| TX | CYD GPIO3 RX |
| GND | System GND |
| VCC | Supply matching the breakout's documented input rating |
| RX, PPS | Leave disconnected |

The CYD receives NMEA at **9600 baud, 8N1**. Leave CYD GPIO2 disconnected in
this build. Remove the old direct GPS-to-Heltec wiring; positions can be shared
through the existing CYD/Heltec serial link using **Use CYD GPS**. Follow
[GPS sharing](GPS_SHARING.md) when enabling it. Receiver command controls are
unavailable for ATGM336H; no u-blox configuration commands are sent.

## ADS1115 battery sensing only

Remove the MAX17048 and its old battery/I2C wires. Identify the CYD I2C
connector pins by signal label, not wire color or connector orientation.

| ADS1115 | Connection |
| --- | --- |
| VDD | CYD 3V3, never 5V |
| GND | System GND |
| SDA | CYD GPIO16, shared with built-in touch/codec |
| SCL | CYD GPIO15, shared with built-in touch/codec |
| ADDR | GND, selecting address 0x48 |
| A0 | Battery divider midpoint below |
| A1, A2, A3 | GND for defined unused inputs; no external sensors or rail divider |
| ALERT/RDY | Leave disconnected |

```text
Switched cell positive (after switch, before boost)
                 |
              10k, 1%
                 |
                 +------------ ADS1115 A0
                 |                |
              10k, 1%           100 nF
                 |                |
System GND ------+----------------+
```

Use two 10k 1% resistors and a 100 nF ceramic filter capacitor. Add a 100 nF
VDD-to-GND decoupling capacitor near the ADC if its breakout lacks one.
Any SDA/SCL pullups must go to 3.3V. Do not connect the raw cell or boosted
5V directly to an ADC input. Keep CYD R14/R15 and GPIO9 untouched.

The existing firmware still samples A1/A2; grounded unused inputs can show
0 V / 0% light. Those values do not establish that monitoring hardware is
installed. To disable polling the absent DHT22/AM2302, set
`#define AM2302_DATA_PIN -1` in `config/device_config.local.h` (copy the example
configuration first if needed). This wiring guide does not change firmware.

## Checks before final assembly

1. With power disconnected, check resistor values, ground continuity and solder
   bridges. Attach the Heltec LoRa antenna before operating the radio.
2. Before connecting A0, power the circuit and measure the divider midpoint:
   it must be half the cell voltage, e.g. 4.2V cell gives 2.1V at A0.
3. Confirm ADS1115 VDD is approximately 3.3V and I2C pullups are at 3.3V.
4. Check switch-off and charger/CYD/Heltec USB combinations. A0 must never
   remain live while ADS1115 VDD is off; adjust the switched sense point if it does.
5. Disconnect all power before attaching A0. On bring-up, expect I2C addresses
   0x48 (ADC), 0x38 (touch), and 0x18 (codec).
6. Compare displayed cell voltage with a meter. Percentage is a voltage estimate;
   this build has no fuel gauge, charging detection or runtime estimate.
7. Check Heltec connectivity and obtain a fresh GPS fix outdoors. Built-in
   screen, touch, MicroSD and speaker connections need no external GPIO wiring.

The [battery reference](BATTERY_STATS.md) retains details for the expanded
sensor variant; its A1/A2 and outside-sensor additions do not apply to this build.

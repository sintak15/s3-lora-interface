# Heltec LoRa Interface

This project turns an ESP32-S3 touch display, often called a CYD, into a
handheld screen and web interface for a Heltec Meshtastic LoRa device.

The CYD shows messages, nodes, GPS/map data, battery information, and radio
diagnostics. It talks to the Heltec over a serial connection.

## Use Case

This project is for situations where phones and normal internet service are not
something you want to depend on. A Meshtastic LoRa radio can pass short messages
and location updates directly between nearby radios, even when there is no cell
signal, WiFi, or cloud service available.

The Heltec board does the radio work. The ESP32-S3 touch display gives that
radio a small handheld dashboard: you can see incoming messages, nearby nodes,
GPS position, battery state, radio health, and offline map information without
digging through a phone app or plugging into a computer.

In plain terms, it is a portable status screen for an off-grid radio network.
It could be useful for camping, hiking groups, event crews, neighborhood
emergency planning, field projects, or any small team that wants a simple way to
see who is nearby and whether messages are moving.

## Quick Reference

This section is for people who already know the hardware terms and just need the
pinout and configuration.

### Hardware

- ESP32-S3 touch display / CYD
- Heltec LoRa device running Meshtastic
- GPS module
- LiPo battery
- LiPo charger module
- 5V boost converter
- Optional MAX17048 LiPo fuel gauge
- Optional MicroSD card

### Power

- LiPo -> LiPo charger
- LiPo charger output -> 5V boost converter input
- Boost converter 5V -> Heltec `5V`, CYD `5V`, GPS `VCC`
- Boost converter GND -> common ground for Heltec, CYD, GPS, and MAX17048

### MAX17048

The MAX17048 monitors the raw LiPo cell before the charger/booster.

- MAX17048 cell positive -> raw LiPo positive
- MAX17048 cell negative -> raw LiPo ground
- MAX17048 `SDA` -> CYD `GPIO16`
- MAX17048 `SCL` -> CYD `GPIO15`
- MAX17048 `GND` -> common ground
- MAX17048 `VCC` -> CYD `3V3`, unless your breakout requires something else
- I2C address: `0x36`

### CYD To Heltec Serial

- CYD `GPIO14` RX -> Heltec `GPIO39` TX
- CYD `GPIO21` TX -> Heltec `GPIO38` RX
- CYD GND -> Heltec GND

Heltec Meshtastic serial module:

- Enabled: yes
- Mode: `PROTO` / `Protobuf`
- Baud: `115200`
- RX pin: `38`
- TX pin: `39`

### GPS

- GPS `TX` -> Heltec `GPIO5` RX
- GPS `TX` -> CYD `GPIO3` RX
- GPS `RX` -> Heltec `GPIO4` TX
- GPS `VCC` -> boosted 5V
- GPS `GND` -> common ground

Heltec Meshtastic GPS pins:

- GPS RX pin: `5`
- GPS TX pin: `4`

### SD Card

The LCDWiki ESP32-S3 display uses its built-in MicroSD slot:

- SD CLK -> CYD `GPIO38`
- SD CMD -> CYD `GPIO40`
- SD D0 -> CYD `GPIO39`
- SD D1 -> CYD `GPIO41`
- SD D2 -> CYD `GPIO48`
- SD D3 -> CYD `GPIO47`

### Offline Map Tiles

Offline map tiles go on the SD card here:

`/s3-lora/tiles/{zoom}/{x}/{y}.rgb565`

Supported zoom levels: `10` through `14`.

Each tile is raw 256x256 RGB565 and should be `131072` bytes.

Example helper-script command:

```powershell
python .\tools\prepare_map_tiles.py `
  --bbox "-97.80,30.10,-97.60,30.35" `
  --zooms "10-14" `
  --output ".\sdcard\s3-lora\tiles" `
  --url-template "https://your-tile-server.example/{z}/{x}/{y}.png"
```

Then copy `.\sdcard\s3-lora\tiles` to the SD card as `/s3-lora/tiles`.

### Web UI

1. Flash `s3-lora-interface.ino` to the CYD.
2. Connect to the CYD WiFi network.
3. Use password `12345678`.
4. Open `http://192.168.4.1/`.

The WiFi network name comes from the selected device config, such as `jjs-node`
or `babs-node`.

## Plain-Language Walkthrough

This section explains the same wiring in more everyday terms.

### What This Device Does

The Heltec is the radio. It sends and receives Meshtastic LoRa traffic.

The CYD is the screen and user interface. It shows messages, known nodes, GPS
information, battery status, maps, and diagnostics.

The GPS provides location data.

The battery, charger, and booster provide portable power.

The optional MAX17048 measures the LiPo battery so the screen can show a more
useful battery percentage.

### Power Wiring

The LiPo battery should not be connected straight to the Heltec or CYD. Instead,
power flows through a charger board, then through a booster that makes a steady
5V output.

Wire it in this order:

1. Connect the LiPo battery to the LiPo charger board.
2. Connect the output of the charger board to the input of the 5V boost
   converter.
3. Connect the boost converter's `5V` output to the Heltec `5V` pin.
4. Connect the boost converter's `5V` output to the CYD `5V` pin.
5. Connect the boost converter's `5V` output to the GPS `VCC` or `5V` pin.
6. Connect all grounds together.

The ground connection is important. The Heltec, CYD, GPS, charger, booster, and
MAX17048 all need to share the same ground.

### Battery Gauge Wiring

The MAX17048 watches the LiPo battery directly. It should measure the raw
battery, not the boosted 5V output.

Wire it like this:

1. Connect the MAX17048 battery positive pad to the LiPo positive side.
2. Connect the MAX17048 battery negative pad to LiPo ground.
3. Connect MAX17048 `SDA` to CYD `GPIO16`.
4. Connect MAX17048 `SCL` to CYD `GPIO15`.
5. Connect MAX17048 `GND` to the shared ground.
6. Connect MAX17048 `VCC` to CYD `3V3`, unless the board you bought says to use
   a different logic voltage.

After flashing the firmware, the CYD Battery page should show the MAX17048
status, battery voltage, and percentage.

### CYD To Heltec Wiring

The CYD talks to the Heltec using serial. Serial uses two signal wires: one side
transmits while the other side receives.

That means TX connects to RX, and RX connects to TX.

Wire it like this:

1. Connect CYD `GPIO14` to Heltec `GPIO39`.
2. Connect CYD `GPIO21` to Heltec `GPIO38`.
3. Connect CYD ground to Heltec ground.

Then open the Meshtastic app and set the Heltec serial module:

1. Turn serial on.
2. Set mode to `PROTO` or `Protobuf`.
3. Set baud to `115200`.
4. Set RX pin to `38`.
5. Set TX pin to `39`.

### GPS Wiring

The GPS sends location text from its `TX` pin. Both the Heltec and the CYD read
that GPS output.

Wire it like this:

1. Connect GPS `TX` to Heltec `GPIO5`.
2. Connect GPS `TX` to CYD `GPIO3`.
3. Connect GPS `RX` to Heltec `GPIO4`.
4. Connect GPS `VCC` or `5V` to the boosted 5V output.
5. Connect GPS `GND` to the shared ground.

Then open the Meshtastic app and set the Heltec GPS pins:

1. Set GPS RX pin to `5`.
2. Set GPS TX pin to `4`.

The CYD reads GPS data directly on `GPIO3` for its GPS screen and map.

### SD Card And Maps

The SD card is optional. The firmware can use it for logs, saved status files,
position history, and offline map tiles.

Offline maps are made of many small tile files. The firmware looks for them in
this folder on the SD card:

`/s3-lora/tiles`

Use zoom levels `10` to `14`. Lower numbers show a wider area. Higher numbers
show more detail.

### Using The Web Page

After flashing the CYD:

1. Look for the WiFi network from the CYD.
2. Connect using password `12345678`.
3. Open `http://192.168.4.1/` in your browser.

The web page shows radio stats, events, chat messages, known nodes, battery
status, SD downloads, and the node map.

## Memory Note

This project uses ESP32-S3 PSRAM for large UI data, logs, and the map canvas.

In the active Arduino `lv_conf.h`, LVGL should be configured to use PSRAM:

- Set `LV_MEM_CUSTOM` to `1`
- Use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` for
  `LV_MEM_CUSTOM_ALLOC`

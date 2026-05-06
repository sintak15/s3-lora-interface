# Heltec LoRa Interface (s3-lora-interface)

This sketch turns an ESP32-S3 board into a handheld interface for a Heltec LoRa unit running Meshtastic firmware.

## Wiring

Cross the UART pins between the ESP32-S3 screen board and the Heltec serial module.

- ESP32-S3 `GPIO14` RX -> Heltec `GPIO39` TX
- ESP32-S3 `GPIO21` TX -> Heltec `GPIO38` RX
- ESP32-S3 GND -> Heltec GND

Set the Heltec Meshtastic serial module to:

- Serial enabled: yes
- Mode: PROTO / Protobuf
- Baud: `115200`
- RX pin: `38`
- TX pin: `39`

Power the GPS module from the Heltec, and split only the GPS TX signal so both
boards can listen:

- GPS TX -> Heltec `GPIO41` RX and ESP32-S3 display `GPIO3` RX
- GPS RX -> Heltec `GPIO42` TX
- GPS VCC -> Heltec 3V3 or a suitable GPS power pin
- GPS GND -> Heltec GND and ESP32-S3 display GND

Set the Heltec Meshtastic GPS pins to:

- GPS RX pin: `41`
- GPS TX pin: `42`

The display reads GPS NMEA directly on `GPIO3`. It does not consume Heltec
position packets for its GPS screen or on-device map.

Optional SD card storage uses the built-in MicroSD socket:

- SD CLK -> ESP32-S3 `GPIO38`
- SD CMD -> ESP32-S3 `GPIO40`
- SD D0-D3 -> ESP32-S3 `GPIO39`, `GPIO41`, `GPIO48`, `GPIO47`

These are configured in `constants.h` for the LCDWiki board's SDMMC/SDIO slot.

Offline on-device map tiles can be copied to the SD card as raw RGB565 files:

- Tile size: 256 x 256 pixels
- Format: raw little-endian RGB565, no header
- Path: `/s3-lora/tiles/<zoom>/<x>/<y>.rgb565`

When the current GPS tile is present, the Node Map page draws it under the node
dots. The firmware checks zoom 14 down through zoom 10 and uses the most
detailed local tile available. If no tile is present, the page still shows GPS
status and plotted dots.

To build a larger offline map pack, use the helper script from this repo:

```powershell
python -m pip install pillow
python tools/make_sd_tiles.py --center 35.1234 -90.1234 --radius-km 5 --zoom 10-14 --out E:\ --source-dir C:\path\to\xyz_tiles
```

`--out` should be the SD card drive root, or any folder you want to copy from.
The script writes the firmware's required `/s3-lora/tiles/...` structure.

You can also pass `--bounds SOUTH WEST NORTH EAST` instead of `--center` and
`--radius-km`. Use `--dry-run` first for large areas to estimate tile count and
raw RGB565 storage:

```powershell
python tools/make_sd_tiles.py --bounds 24.396308 -124.848974 49.384358 -66.885444 --zoom 10-14 --out E:\ --dry-run
```

If using `--tile-url`, only use a tile service that explicitly allows offline
caching or bulk tile creation. The public `tile.openstreetmap.org` service is
for live map viewing and should not be used to predownload larger offline packs.

You can also render simple street maps directly from an OpenStreetMap `.osm.pbf`
file, with no web tile downloading. 
```

Use `--bounds SOUTH WEST NORTH EAST` to narrow or expand the render area. The
script writes directly to `/s3-lora/tiles/...` on the SD card.

## Use

1. Flash `s3-lora-interface.ino`.
2. Connect to WiFi network `Heltec-LoRa-Interface`.
3. Use password `12345678`.
4. Open `http://192.168.4.1/`.

The page shows radio stats, decoded events, chat messages, an OpenStreetMap node map with an offline fallback plot, SD log downloads, and known nodes. The send box broadcasts a Meshtastic text message to the mesh.

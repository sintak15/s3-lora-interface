# Heltec LoRa Interface

A touchscreen and local web interface for a Heltec Meshtastic radio, built on
an ESP32-S3 CYD. Read messages, check radio and battery status, browse nodes,
and use offline maps without depending on internet access.

**Current firmware: v0.1.94.** The CYD runs this firmware; the Heltec runs
Meshtastic and handles the LoRa radio connection.

## Pin mapping — quick reference

For the **LCDWiki 2.8-inch ESP32-S3 CYD** and **Heltec V3.2**. Numbers below
are **GPIO numbers on the named board**, not physical header positions.
All modules share a common ground; UART and I²C signals use 3.3 V logic.

| CYD pin / terminal | Connect to | Purpose |
| --- | --- | --- |
| **GPIO14 — RX** | **Heltec GPIO39 — TX** | Heltec → CYD, 115200 baud |
| **GPIO21 — TX** | **Heltec GPIO38 — RX** | CYD → Heltec, 115200 baud |
| **GPIO3 — GPS RX** | **GPS TX**; split that same GPS TX wire to **Heltec GPIO5 — GPS RX** | GPS → both boards, 9600 baud |
| **GPIO2 — GPS TX** | **GPS RX** | CYD → GPS configuration, 9600 baud |
| **GPIO16 — SDA** | **MAX17048 SDA** | Shared I²C data; gauge address `0x36`; also used by built-in touch and audio codec |
| **GPIO15 — SCL** | **MAX17048 SCL** | Shared I²C clock |
| **GND** | Heltec GND, GPS GND, MAX17048 GND and power ground | Common ground |
| **5V** | Boost-converter 5V output; same rail supplies Heltec 5V and GY-NEO6MV2 VCC | Board / breakout power, separate from UART logic |
| **3V3** | MAX17048 breakout VCC, subject to its supply requirements | Gauge supply; cell sensing connects to the raw battery |

**Leave Heltec GPIO4 disconnected from GPS RX.** Only CYD GPIO2 drives that
wire. Enable the Heltec Serial module in **PROTO / Protobuf** mode at
**115200 baud**, with **RX 38 / TX 39**. Enable GPS with **GPS RX 5**.
See [detailed wiring and battery connections](#wiring) below.

These built-in CYD connections are already routed on the board:

| Built-in function | CYD GPIO assignment |
| --- | --- |
| TFT | CS **10**, DC **46**, MOSI **11**, SCLK **12**, MISO **13**, backlight **45** |
| Touch | SDA **16**, SCL **15**, reset **18**, interrupt **17**; I²C address `0x38` |
| MicroSD | CLK **38**, CMD **40**, D0 **39**, D1 **41**, D2 **48**, D3 **47** |
| Audio clocks | MCLK **4**, BCLK **5**, LRCLK **7** |
| Audio data | **8** ESP32 → codec playback input; **6** codec → ESP32 recording output |
| Audio control | Amplifier enable **1**, active low; codec I²C address `0x18` |

Playback uses GPIO8, matching the
[board schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf).
Connect the speaker to the CYD's dedicated speaker port; these GPIO assignments
describe internal board wiring. See [notification audio](docs/AUDIO.md).

## What it does

- **Home dashboard:** radio status, battery level, node/GPS status and a message
  preview. Tap a card for details, or Menu for the launcher.
- **Messaging:** channel and direct messages, unread counts, delivery states,
  and retry controls where available.
- **Notification audio:** soft boops and bubble pops with separate message/direct
  selections, clean volume control, dedicated playback and automatic fault recovery.
- **Offline maps:** SD-backed maps on both the touchscreen and web interface,
  zooms 10–14, feet/miles scales and coarser fallback when detail is missing.
- **Map navigation:** drag to explore, pause/follow the device, recenter and
  inspect actual coverage around the touchscreen map center.
- **Battery monitoring:** optional MAX17048 fuel-gauge support, charging and
  low-battery status, and runtime estimates with discharge learning retained
  across restarts. See [battery statistics](docs/BATTERY_STATS.md).
- **Local storage:** chat/event logs, position history, saved location and a
  cached map image on MicroSD.
- **USB tools:** upload tiles, copy files to a replacement card and inspect
  receiver-level GPS and audio diagnostics.

<img src="docs/images/home.png" alt="Touchscreen home dashboard with radio, battery, nodes and messages cards" width="240">

The launcher contains Messages, Nodes, Map, TX Log and System. Health is under
System. See the [home dashboard guide](docs/HOME_DASHBOARD.md) for details.

## Hardware

The current build uses:

| Component | Purpose |
| --- | --- |
| [LCDWiki 2.8-inch ESP32-S3 display / CYD](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display) | Touchscreen, web interface and SD storage |
| Heltec V3.2 running Meshtastic | LoRa radio |
| GY-NEO6MV2 GPS module | Location input shared by the Heltec and CYD |
| Single-cell rechargeable battery, 2500 mAh 18650 in this build | Portable power |
| TP4056 charger and MT3608 5V boost converter | Charging and boosted supply |
| MAX17048 fuel-gauge breakout, optional | Battery state measurement |
| FAT32 MicroSD card, optional | Offline maps and persistent logs |

The verified CYD has 16 MiB flash and 8 MiB PSRAM. Pin assignments below are
specific to this hardware. GPIO numbers are not physical header positions.

## Enclosure

[V4 OpenSCAD enclosure](hardware/enclosure/enclosure_v4.scad) uses measured component envelopes,
the single-battery holder, intact power assembly and oval speaker. It provides
visible CYD and Heltec screens, module mounts and separate charging/USB access.
Printed antenna and speaker screw retainers provide mounting without foam.
See the [V4 enclosure guide](docs/ENCLOSURE_V4.md) for previews, exports and
assembly. Print the fit coupon and verify connector/mount offsets before a full
case. The [V3 prototype](docs/ENCLOSURE.md) remains available separately.

Latest fit revisions (earlier STL iterations are retained for reference):

Enclosure sources, printable files and fit coupons are grouped under
[`hardware/enclosure/`](hardware/enclosure/README.md).

- [Current face: tapered GPS supports and 1.8 mm CYD pilots](hardware/enclosure/enclosure-v4-front-gps-tapered-rims.stl).
- [Current base plus two Heltec brackets on one 180 mm plate](hardware/enclosure/enclosure-v4-latest-base-and-heltec-brackets.stl).
- [Current base alone: MAX screw posts and open battery area](hardware/enclosure/enclosure-v4-base-max-screw-posts.stl).
- [Matching antenna brackets](hardware/enclosure/enclosure-v4-antenna-rails-repair.stl).
- [Speaker retaining frame](hardware/enclosure/enclosure-v4-speaker-frame.stl).

Print at 100% scale in millimetres, keeping the supplied bed orientation.
See the [latest face notes](docs/GPS_TAPERED_RIMS.md) and
[base mounting notes](docs/MAX_SCREW_POSTS.md) for screw sizes and fit checks.
The current designs have geometry checks but are still physical-fit prototypes.

## Build and first connection

1. Install the pinned Arduino dependencies and board configuration from
   [docs/BUILD.md](docs/BUILD.md). LVGL and TFT_eSPI configuration files are
   included in the repository.
2. To customize the device identity and credentials, copy
   `config/device_config.example.h` to `config/device_config.local.h` and edit
   that local file. Git ignores it. Without it, the example configuration is used.
3. Wire and configure the Heltec serial connection as shown below. Enable its
   GPS and configure the GPS pins if the receiver is installed.
4. Build the CYD firmware without changing its version:

   ```powershell
   .\tools\build_ota_bin.ps1 -NoVersionBump
   ```

   The binary is written under `dist/`. You can also use Arduino IDE with the
   board settings documented in the build guide.
5. Flash the CYD, connect to its Wi-Fi access point, and open
   **http://192.168.4.1/**. In station mode, use its assigned local-network address.

With the unchanged example configuration, the Wi-Fi network is
`s3-lora-interface`, the Wi-Fi password is `setup1234`, and the web login is
`admin` / `setup1234`. Complete the first-run setup and change the password.
Custom configurations use their configured network name and credentials.

The web interface includes messaging, nodes, offline maps, health, settings,
SD downloads and firmware upload. The map library and stylesheet are bundled
in firmware, so the web map needs no internet access.

## Wiring

TX connects to RX: one board's transmit wire goes to the other's receive pin.
All boards share a common ground.

### Power and fuel gauge

The documented power path is battery → charger output → 5V boost converter.
The boosted rail supplies the Heltec `5V`, CYD `5V` and GY-NEO6MV2 breakout
`VCC`. The CYD USB port does not charge this project's battery; charging uses
the separate charger connection.

The MAX17048 measures the **raw cell**, before the boost converter:

| MAX17048 connection | Connect to |
| --- | --- |
| Cell positive | Raw battery positive |
| Cell negative / GND | Common ground |
| SDA | CYD GPIO16 |
| SCL | CYD GPIO15 |
| VCC | CYD 3V3, subject to the breakout's supply requirements |

The gauge uses I²C address `0x36`. Its battery reading does not measure the
boosted GPS supply rail. Without a gauge, the UI shows an unavailable reading.

### CYD ↔ Heltec serial

| CYD | Heltec |
| --- | --- |
| GPIO14 — RX | GPIO39 — TX |
| GPIO21 — TX | GPIO38 — RX |
| GND | GND |

In the Meshtastic serial module, set **Enabled**, **PROTO / Protobuf** mode,
**115200 baud**, **RX pin 38** and **TX pin 39**.

### GPS

Split GPS TX to both receivers. Only the CYD drives GPS RX to configure the
receiver while the Heltec only listens:

| GPS module | Connect to |
| --- | --- |
| TX | Heltec GPIO5 **and** CYD GPIO3 |
| RX | CYD GPIO2 (GPS TX) |
| VCC | Boosted 5V rail for the GY-NEO6MV2 breakout |
| GND | Common ground |

Enable GPS in Meshtastic Position settings and set **GPS RX pin 5**.
**Leave Heltec GPIO4 disconnected from GPS RX**; never tie both boards' TX
outputs to the GPS RX wire. An existing Heltec TX-pin setting does not change
this receive-only physical wiring.

The CYD receives on GPIO3 and transmits on **GPIO2**, at **9600 baud**.
GPIO2 is the unused expansion IO alongside GPIO3/14/21; see the
[LCDWiki board manual](https://www.lcdwiki.com/res/ES3C28P/2.8inch_IPS_ESP32-S3_ES3C28P_ES3N28P_User_Manual.pdf).
Boot sends no GPS configuration commands. USB `gps rate` queries the receiver;
`gps rate 1`, `gps rate 2` and `gps rate 5` request measurement rates with
acknowledgement and readback reporting. Prefer 1 Hz for the shared 9600-baud
stream; higher rates may exceed its NMEA bandwidth. Use compatible 3.3 V UART signals and common
ground; the breakout's boosted 5V power input is separate from UART logic.
The GPS and radio serial connections use different pins and baud rates.
See [GPS wiring and control ownership](docs/GPS_DIAGNOSTICS.md#gps-wiring-and-control-ownership)
for commands, verification and control ownership.

### Built-in MicroSD slot

| SD signal | CYD GPIO |
| --- | --- |
| CLK | 38 |
| CMD | 40 |
| D0 | 39 |
| D1 | 41 |
| D2 | 48 |
| D3 | 47 |

Use the CYD's built-in slot. These are CYD GPIO assignments, separate from
the similarly numbered Heltec pins.

## Offline maps

Place tiles on the SD card using this structure:

```text
/s3-lora/tiles/{zoom}/{x}/{y}.rgb565
```

Each tile is **256×256 RGB565, little-endian, exactly 131072 bytes**. Supported
zoom levels are **10–14**; lower numbers show a wider area. Preparing new maps
requires a suitable tile source; viewing installed maps is fully offline.

On the touchscreen, use −/+ to change zoom, drag to explore, and use Follow
or My location to resume tracking. Coverage reports installed tiles around the
current center at each level. The requested zoom is retained when a coarser
view is needed. Availability is checked from files, not inferred from a
download plan.

The browser uses the same SD tiles. It preserves your pan position during
status updates and provides My location, Show nodes and Refresh maps controls.
Its Touchscreen coverage card describes the touchscreen's last checked area;
the browser map has its own visible-tile status.

See [touchscreen navigation](docs/MAP_NAVIGATION.md),
[coverage and fallback](docs/MAP_COVERAGE.md), and
[offline browser maps](docs/WEB_OFFLINE_MAPS.md).

### Upload tiles over USB

Install `pyserial`, connect through the **CYD USB port**, and close other serial
monitors. Replace `COM12` and `H:` with the actual port and card-reader drive.

```powershell
python -m pip install pyserial
python tools/upload_tiles_usb.py --port COM12 --tiles H:\s3-lora\tiles
```

The tile folder must contain `z/x/y.rgb565`. Identical tiles are verified and
skipped; add `--overwrite` to replace different existing tiles. Transfers use
CRC checks and SD readback verification. Use `--dry-run` to validate local
paths and sizes without connecting.

For large map packs, downloading directly to a card in a USB reader avoids the
CYD transfer step. The USGS downloader supports resuming and a storage budget,
with a St. Paul-centered coverage profile. See the
[map transfer and replacement-card guide](docs/USB_MAP_TRANSFER.md) for tile
preparation, direct downloads, source attribution and safe stopping/resuming.

### Preserve files when changing cards

To copy supporting files while preparing a new map pack separately:

```powershell
python tools/copy_sd_usb.py --port COM12 --destination H:\ `
  --manifest build/sd-copy-H.jsonl --progress build/sd-copy-H.json `
  --exclude-path /s3-lora/tiles
```

This reads the installed card and preserves directory structure on the new
card. Omit `--exclude-path` to copy existing tiles too. Supporting files can
include chat history and saved locations. Shut down the CYD before swapping
cards, and keep the original card until the replacement is working.

## GPS diagnostics

Open **System → GPS** to inspect receiver bytes, valid NMEA sentences,
checksum errors and available satellite counts before a position fix. Unknown
satellite counts are distinct from zero.

Over the CYD USB connection, the `gps` command reports the same receiver health
and the Heltec's reported GPS pin settings. To record an observation:

```powershell
python tools/gps_diagnostics_usb.py --port COM12 --duration 35 --interval 5 `
  --output build/gps-observation.json
```

The packet-statistics **GPS / Positions** counter measures mesh position
messages, including other nodes. It is not a receiver-fix counter.

| Observation | Next checks |
| --- | --- |
| Receiver bytes stay at zero | GPS power, common ground and GPS TX → CYD GPIO3 |
| Bytes arrive, but valid NMEA does not increase | Baud rate, output protocol and signal integrity |
| Valid sentences arrive, but no fix | Outdoor sky visibility, antenna connection/orientation and GPS supply stability |

The latest recorded observation confirmed valid serial data and the expected
Heltec pin settings, but did **not** acquire a position fix. See
[GPS diagnostics and recorded findings](docs/GPS_DIAGNOSTICS.md) for details.

## Memory and development

Large UI buffers, logs and the map canvas use ESP32-S3 PSRAM. The optional
eight-tile cache holds **1 MiB** in PSRAM; display transfer buffers and task
stacks remain in internal RAM. See the
[measured cache profile](docs/MAP_CACHE_PROFILE.md) for timings and limits.

Build dependencies, verification commands, source layout and hardware checks
are documented in [docs/BUILD.md](docs/BUILD.md). Local device configuration,
generated build identity, binaries, logs and download progress remain outside
Git under the configured ignore rules.

See the [documentation index](docs/README.md) for current guides and historical
reviews, and [contribution conventions](CONTRIBUTING.md) for naming, file
placement and verification before committing.

The bundled Leaflet viewer retains its license in
[src/vendor/leaflet/LICENSE](src/vendor/leaflet/LICENSE).

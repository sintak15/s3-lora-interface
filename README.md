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

Offline on-device map tiles can be copied to the SD card as raw RGB565 files.

## Use

1. Flash `s3-lora-interface.ino`.
2. Connect to WiFi network `Heltec-LoRa-Interface`.
3. Use password `12345678`.
4. Open `http://192.168.4.1/`.

The page shows radio stats, decoded events, chat messages, an OpenStreetMap node map with an offline fallback plot, SD log downloads, and known nodes. The send box broadcasts a Meshtastic text message to the mesh.

## LVGL memory

This baseline expects LVGL to allocate UI objects from ESP32-S3 PSRAM. In the active Arduino `lv_conf.h`, set `LV_MEM_CUSTOM` to `1` and use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` for `LV_MEM_CUSTOM_ALLOC`. The sketch also moves large logs and the map canvas into PSRAM at boot.

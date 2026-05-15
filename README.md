# Heltec LoRa Interface (s3-lora-interface)

This sketch turns an ESP32-S3 board into a handheld interface for a Heltec LoRa unit running Meshtastic firmware.

## Wiring

Power is supplied from a shared boosted 5V rail:

- LiPo battery -> LiPo charger module
- LiPo charger output -> 5V boost converter
- Boost converter 5V -> Heltec 5V pin, ESP32-S3 display/CYD 5V pin, and GPS 5V/VCC pin
- Boost converter GND -> common ground for Heltec, ESP32-S3 display/CYD, and GPS

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

GPS data is shared between the Heltec and the ESP32-S3 display. The GPS is
powered from the boosted 5V rail, not from the Heltec:

- GPS TX -> Heltec `GPIO41` RX and ESP32-S3 display `GPIO3` RX
- GPS RX -> Heltec `GPIO42` TX
- GPS VCC -> boosted 5V rail
- GPS GND -> common ground

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

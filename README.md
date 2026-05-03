# Heltec LoRa Interface

This sketch turns an ESP32-S3 board into a browser-based interface for a Heltec LoRa unit running Meshtastic firmware.

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

Wire the GPS module to the Heltec:

- GPS TX -> Heltec `GPIO41` RX
- GPS RX -> Heltec `GPIO42` TX
- GPS VCC -> Heltec 3V3 or a suitable GPS power pin
- GPS GND -> Heltec GND

Set the Heltec Meshtastic GPS pins to:

- GPS RX pin: `41`
- GPS TX pin: `42`

## Use

1. Flash `s3-lora-interface.ino`.
2. Connect to WiFi network `Heltec-LoRa-Interface`.
3. Use password `12345678`.
4. Open `http://192.168.4.1/`.

The page shows radio stats, decoded events, chat messages, and known nodes. The send box broadcasts a Meshtastic text message to the mesh.

# Heltec firmware updates

The CYD and Heltec run separate firmware. A CYD application update does not
update Meshtastic or the Heltec's OTA loader. The current CYD firmware can
configure Heltec networking through its application UART, but contains no
Heltec firmware uploader or USB-to-ROM flashing bridge.

## Network update investigation — October 23, 2026

The connected radio identified as HELTEC_V3 running **2.7.15.567b8ea**. Its
normal HTTP diagnostics and Meshtastic TCP service on port 4403 were reachable.
Device configuration, module configuration and eight channel slots were backed
up to the ignored `build/heltec-update/` directory before attempting OTA.
Those backups contain private configuration and must not be committed.

The prepared target was **2.7.26.54e0d8d Beta**, the latest regular release
listed by the official release API at the time. The newer 2.8.1 Alpha introduces
major mesh-protocol changes. The official ESP32-S3 release archive's SHA256
matched its published digest, and the Heltec V3 application image matched its
manifest size and MD5. Only the main application image was selected; no erase,
factory image, filesystem or partition-table operation was attempted.

The old `reboot_ota_seconds` command entered the installed Bluetooth OTA loader.
The network uploader received no response on its UDP port 3232 handshake and
transferred no firmware. BLE inspection of the matching node showed the legacy
OTA service and characteristics; the newer `VERSION` query received no reply.
This behavior matches the old Bluetooth-only loader's `OTA_SIZE:` protocol.
The loader has no Wi-Fi path or automatic return to the application in its
idle loop; a physical reset/power cycle returns to the original application.

After the user power-cycled the Heltec, HTTP and TCP communication recovered.
Readback still reported **2.7.15.567b8ea**. Device settings, module settings and
all eight channel slots exactly matched the pre-attempt backup. The Heltec was
not updated.

## Available paths

| Path | Requirement |
| --- | --- |
| Wi-Fi OTA | A compatible Wi-Fi or unified loader must already be installed and entered with the matching command/protocol |
| Legacy Bluetooth OTA | A client that supports the installed loader's older protocol; current unified OTA commands are different |
| USB / UART0 flashing | A working USB data connection or a suitable 3.3 V serial connection to the ESP32-S3 ROM UART, plus boot/reset handling |
| Flashing through the CYD | Additional bridge firmware and different wiring; the existing application UART is insufficient |

The existing Heltec GPIO38 RX / GPIO39 TX connection is an application UART.
The ROM flashing UART0 is GPIO44 RX / GPIO43 TX on Heltec V3.2; the onboard
CP2102 also connects to those pins. A wired recovery design must account for
that existing driver and boot/reset signals before attaching another transmitter.

Modern unified OTA uses TCP port 3232 and a hash supplied by the main firmware;
it is not the older Arduino-compatible UDP OTA protocol. Do not infer loader
support merely from a working IP address, main-firmware version, or an open
normal Meshtastic TCP port.

Sources: [installed main-firmware OTA command](https://github.com/meshtastic/firmware/blob/567b8ea/src/modules/AdminModule.cpp),
[legacy Bluetooth loader](https://github.com/meshtastic/firmware-ota/blob/master/src/main.cpp),
[legacy Wi-Fi loader](https://github.com/meshtastic/firmware-ota-wifi),
[unified OTA protocol](https://github.com/meshtastic/esp32-unified-ota),
[2.7.26 Beta release](https://github.com/meshtastic/firmware/releases/tag/v2.7.26.54e0d8d),
[2.8.1 Alpha release notes](https://github.com/meshtastic/firmware/releases/tag/v2.8.1.8e6a88d),
[Heltec V3.2 manual](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3.2.pdf).

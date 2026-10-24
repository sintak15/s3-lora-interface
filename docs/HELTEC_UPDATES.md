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

## Bluetooth retry — October 23, 2026

The official release API still identified **2.7.26.54e0d8d Beta** as the latest
regular release. A new private backup confirmed HELTEC_V3, firmware 2.7.15,
external GPS mode and eight channel slots. The application image was checked
against its manifest size/MD5 and the release archive's published SHA256.

A Windows BLE client connected to the matching radio, verified the legacy OTA
service and characteristics, subscribed to notifications and negotiated an ATT
MTU of 517. It sent `OTA_SIZE:<application bytes>`, followed by one application
block. Neither a 512-byte attempt nor a fresh 128-byte attempt received the
expected first-block acknowledgement within 30 seconds. No later blocks were
sent after an unacknowledged write. The user power-cycled the Heltec between
attempts; the second attempt disconnected and normal HTTP/TCP access recovered.

Readback still reported **2.7.15.567b8ea**. Device configuration, module
configuration and all eight channels exactly matched the new backup. Fresh
LOC_EXTERNAL positions confirmed that CYD GPS sharing resumed. USB enumeration
still showed no Heltec data port. This retry did not complete a firmware update.

There is a possible partition-label incompatibility: the legacy loader source
searches OTA_0 for the exact label `app` and asserts if it is absent, whereas
the installed firmware's 8 MB partition definition names that slot `app0`.
That could explain failure on the first data block and a return to normal
operation. This is an inference from source and observed behavior, not a
confirmed crash diagnosis; the installed loader binary and serial panic log
were not read back. Repeating the same wireless transfer is not a reliable
recovery path. A working USB data connection or access to ROM UART0 is needed
to install firmware and a compatible updater.

Retry scripts, transfer status and private before/after configuration captures
remain ignored under `build/heltec-update/` (`retry_*.py`,
`retry-transfer-progress.json`, `retry-pre-update-config.json` and
`retry-after-config.json`). They must not be committed with private settings.

## Second remote node — October 23, 2026

A second Heltec V3 base node also identified as **2.7.15.567b8ea**. Its complete
device/module settings and eight channel slots were backed up before any reboot.
The user confirmed remote power cycling was available. The verified 2.7.26
Heltec V3 application image was prepared, and identity/settings were checked
again before entering the installed updater.

No TCP 3232 update service was reachable after loader entry, and the matching
Bluetooth advertisement was not visible from this PC. A legacy Wi-Fi uploader
opened its TCP return listener before sending an invitation and kept the same
UDP return port open for 40 seconds to allow flash-erasure time. No invitation
response arrived and **zero application-image bytes were transferred**. The
installed loader's identity was not established; absence from a BLE scan does
not prove that it was a Wi-Fi loader or that no Bluetooth loader was installed.

After the user power-cycled this node, normal HTTP and Meshtastic TCP services
returned. Independent readback confirmed the same node identity, firmware
**2.7.15.567b8ea**, and exact matches for device settings, module settings and all
eight channels. This was an unsuccessful wireless update attempt with verified
recovery. No CYD firmware or first-node settings were changed by this attempt.
Private captures remain ignored under `build/heltec-update/node139/`, including
`pre-update-config.json`, `wifi-update-attempt.json`, `after-update-config.json`
and `verification-result.json`.

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
[legacy loader partition lookup](https://github.com/meshtastic/firmware-ota/blob/87f0f2f41bbbe446db1c2bba48c30eb3469a97cb/src/main.cpp),
[installed 8 MB partition definition](https://github.com/meshtastic/firmware/blob/567b8ea/partition-table-8MB.csv),
[legacy Wi-Fi loader](https://github.com/meshtastic/firmware-ota-wifi),
[unified OTA protocol](https://github.com/meshtastic/esp32-unified-ota),
[2.7.26 Beta release](https://github.com/meshtastic/firmware/releases/tag/v2.7.26.54e0d8d),
[2.8.1 Alpha release notes](https://github.com/meshtastic/firmware/releases/tag/v2.8.1.8e6a88d),
[Heltec V3.2 manual](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3.2.pdf).

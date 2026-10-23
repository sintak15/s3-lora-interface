# GPS receiver diagnostics and control

USB command `gps` reports the CYD receiver's raw byte count, last-byte age,
checksum-valid NMEA sentences, checksum failures, fix-bearing sentences,
satellites used, satellites in view when reported, and fresh-fix status. It also
reports the Heltec's received GPS mode/pin configuration and the separate count
of mesh position messages. No coordinates are exported by this command.

The CYD listens on GPIO3 at 9600 baud. The UART receive buffer is 4096 bytes.
Checksum and acquisition counters are now included in the health report, and
System > GPS shows them before a position fix. GPGSV/GNGSV satellite-in-view
fields are parsed independently of a fix. An absent or stale field is unknown,
not zero; satellites used and satellites in view have different meanings.

Observe the receiver with:

```powershell
python tools/gps_diagnostics_usb.py --port COM12 --duration 35 --interval 5 --output build/gps-observation.json
```

The script preserves the running device by opening USB without asserting DTR
or RTS. It records only receiver-health fields and configuration metadata. It
does not change Heltec settings or GPS module configuration.

## GPS wiring and control ownership

The intended wiring is one GPS output feeding two listeners, with only the
CYD allowed to send GPS configuration commands:

```text
NEO-6M TX ----+---- CYD GPIO3 (GPS RX)
             +---- Heltec GPIO5 (GPS RX)

NEO-6M RX ---------- CYD GPIO2 (GPS TX)
Heltec GPIO4 ------- disconnected from GPS RX
All grounds -------- common
```

**Current firmware uses GPIO2 for GPS TX and GPIO3 for GPS RX at 9600 baud.**
GPIO2 is on the expansion connector; GPIO14/21 remain the radio UART.
Both boards still receive GPS data if the return wire is missing, but commands
will time out. Startup sends no GPS configuration commands.

Do not connect Heltec TX and CYD TX together. Disconnect the old Heltec
GPIO4-to-GPS-RX wire when adopting this layout. Meshtastic may still report its
configured TX pin; the report does not prove a physical TX wire is connected.
GPS RX GPIO5 remains the Heltec's receive connection. Use 3.3 V-compatible
UART logic, independently of the breakout's 5V power connection.

### USB control commands

| Command | Action |
| --- | --- |
| `gps` | Print diagnostics without transmitting to the receiver |
| `gps config` | Read rate, navigation, SBAS, power and six NMEA message rates |
| `gps rate` | Poll receiver measurement/navigation-rate settings |
| `gps rate 1` | Request 1000 ms measurement period (1 Hz) |
| `gps rate 2` | Request 500 ms measurement period (2 Hz) |
| `gps rate 5` | Request 200 ms measurement period (5 Hz) |
| `gps save` | Request receiver storage after a complete read or verified apply |

Rate changes use UBX-CFG-RATE with navigation rate 1, preserving time alignment,
following the [u-blox 6 protocol specification, section 31.17](https://content.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_%28GPS.G6-SW-10018%29_Public.pdf).
The CYD waits up to 2 seconds for ACK/NAK, then queries the rate after ACK.
The reported measurement period comes from a receiver reply, not the request.
Polls also time out after 2 seconds. Additional commands are rejected while
one is pending; invalid rates send nothing. Run `gps config` before setting a
rate so the CYD can validate the receiver's power mode and output. Above 1 Hz
requires Compact output and Maximum performance, selected in
[System > GPS > Config > Adv](GPS_CONFIG.md). USB rate commands use the same
validation and never implicitly change power or NMEA output. Start with 1 Hz.

`gps` and health JSON add `gpsTxPin`, `gpsControlState`,
`gpsMeasurementPeriodMs`, `gpsUbxReplies` and `gpsUbxChecksumErrors`. A null
period means the rate is not currently confirmed. Additional fields report
`gpsConfigBusy`, `gpsConfigKnownMask`, `gpsNavigationModel`, `gpsSbasEnabled`,
`gpsReceiverPowerMode`, `gpsNmeaProfile`, `gpsConfigCanSave` and `gpsConfigError`.
The known-mask bits 0–9 correspond to rate, navigation, SBAS, power, GGA, GLL,
GSA, GSV, RMC and VTG. UBX frames are
checksum checked and kept out of NMEA parsing; truncated frames reset after
a 250 ms receive gap. Payload lengths are capped at 256 bytes, retaining only
36 bytes for NAV5 and the smaller supported replies. The error counter also
includes rejected oversize UBX headers.

Config can explicitly change NMEA output and request receiver storage, as
described in the [configuration guide](GPS_CONFIG.md). No baud changes or
automatic startup writes are sent. Retention depends on the module's storage
and backup power. Adding TX alone does not improve GPS accuracy.

### Wiring verification

1. Disconnect Heltec GPIO4 from GPS RX before connecting CYD GPIO2.
2. Connect GPS TX to both receive pins, with common ground and compatible logic.
3. Run `gps` and confirm bytes and valid NMEA sentences continue increasing.
4. Run `gps rate` and check the returned measurement period. A timeout with
   working NMEA suggests the new transmit wire or disabled UBX input.
5. Run `gps config` before an optional `gps rate 1`, then check ACK followed by
   1000 ms readback. Leave Save alone when doing a temporary wiring check.

Compilation and protocol tests do not verify the physical wiring or receiver.

## Connected-device observation (2026-10-17)

This historical observation used the earlier Heltec-controlled GPS wiring;
the reported GPIO4 TX setting is not the new wiring recommendation above.

The Heltec link was responsive with zero decode errors. The previously observed
35–38 GPS packets were mesh position messages, not receiver serial sentences.
After installing the diagnostic build, a 28.8-second observation measured:

- 2343 additional raw GPS bytes.
- 70 additional checksum-valid NMEA sentences.
- Zero checksum failures and zero fix-bearing sentences.
- Zero satellites used; satellites-in-view data was not reported.
- Heltec GPS enabled, RX GPIO5 and TX GPIO4, matching the project wiring.

This confirms working serial communication during the observation, with no
receiver fix. It does not establish antenna condition, sky visibility, GPS
supply voltage or supply-noise levels. The next physical checks are outdoor
sky visibility, antenna seating/orientation and stable GPS power. The CYD
battery measurement does not measure the boosted GPS supply rail.

The build and existing browser regression checks passed before installation;
the flasher verified its application hash. Further testing was stopped at the
user's request. The observation is saved locally as
`build/v0.1.90-gps-observation.json`.

Artifact: `dist/s3-lora-interface-v0.1.90-verified-20261017.bin` (1857536 bytes).
SHA256: `d30e114e880f4e29f2fa4c84875518ad504e534950f4577fdbac488120c89e48`.
Pre-flash metadata backup: `build/pre-v0.1.90-flash-metadata.bin`.

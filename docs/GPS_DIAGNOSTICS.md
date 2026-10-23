# GPS receiver diagnostics (v0.1.90)

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
CYD allowed to send GPS configuration commands in a future firmware update:

```text
NEO-6M TX ----+---- CYD GPIO3 (GPS RX)
             +---- Heltec GPIO5 (GPS RX)

NEO-6M RX ---------- CYD GPS TX (future; GPIO not assigned)
Heltec GPIO4 ------- disconnected from GPS RX
All grounds -------- common
```

**Current firmware remains receive-only:** `GPS_TX_PIN` is `-1`, the baud rate
is 9600, and no CYD UBX command/configuration support is introduced by this
documentation update. Leave NEO-6M RX unconnected until a CYD TX GPIO is chosen
and transmit support is implemented; insulate any reserved wire end.
Both boards can receive position data without that return connection.

Do not connect Heltec TX and CYD TX together. Disconnect the old Heltec
GPIO4-to-GPS-RX wire when adopting this layout. Meshtastic may still report its
configured TX pin; the report does not prove a physical TX wire is connected.
GPS RX GPIO5 remains the Heltec's receive connection. Use 3.3 V-compatible
UART logic, independently of the breakout's 5V power connection.

The future CYD control path would allow update-rate, output-message and serial
configuration. Any baud or message changes must remain compatible with both
listeners. Merely adding TX does not improve fix accuracy or acquisition.
The `gps` diagnostic command still observes the stream without sending commands.

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

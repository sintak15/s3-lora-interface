# ATGM336H GPS diagnostics

The ATGM336H sends receive-only NMEA to CYD GPIO3 at **9600 baud, 8N1**.
Connect GPS TX to GPIO3 and GND to system ground; supply VCC according to the
actual breakout rating. Leave GPS RX, PPS and CYD GPIO2 disconnected. There is
no direct GPS-to-Heltec wire. See the [complete wiring guide](WIRING.md).

USB command `gps` reports raw byte count, last-byte age, checksum-valid NMEA
sentences, checksum failures, fix-bearing sentences, satellites used, satellites
in view when reported, and fresh-fix status. Mesh position messages are counted
separately and do not prove local GPS reception. System > GPS displays receiver
health before a fix; missing or stale satellite data is unknown rather than zero.

Observe reception without changing GPS or Heltec settings:

```powershell
python tools/gps_diagnostics_usb.py --port COM12 --duration 35 --interval 5 --output build/gps-observation.json
```

Replace COM12 with the device's actual port. The script opens USB without
asserting DTR/RTS and records health/configuration metadata without coordinates.

1. Check common ground, breakout supply and TX-to-GPIO3 wiring.
2. Run `gps` and confirm raw bytes and checksum-valid NMEA counts increase.
3. Place the antenna outdoors with a clear sky view; first acquisition can take
   several minutes. Confirm a fresh fix and map position.
4. If bytes arrive without valid sentences, inspect a short UART sample for
   baud, GP/GN talker IDs and checksum integrity.
5. Enable sharing separately using the [Heltec GPS sharing guide](GPS_SHARING.md).

`gps config`, `gps rate` and `gps save` report unsupported without transmitting
receiver commands. Changing these controls cannot fix ATGM336H reception.
`gps share setup`, `gps share on` and `gps share off` control Heltec sharing.
Compilation and protocol tests do not establish physical reception or antenna fit.

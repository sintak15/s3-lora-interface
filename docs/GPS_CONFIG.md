# ATGM336H GPS configuration

The CYD receives GP/GN NMEA from the ATGM336H on GPIO3 at **9600 baud, 8N1**.
GPS RX and CYD GPIO2 remain disconnected in the core build; `GPS_TX_PIN=-1`.
Use the [wiring guide](WIRING.md) for power and signal connections.

Position reception, maps, fix diagnostics and Heltec sharing are available.
Receiver read/apply/save, rate, navigation model, power and SBAS controls are
unsupported for this receiver. The firmware sends no receiver configuration
commands. The API reports `gpsReceiver=ATGM336H` and `gpsConfigSupported=false`.
Keep `GPS_RECEIVER_UBLOX=0`, the default for the installed hardware.

**System > GPS > Config > Adv > Use CYD GPS** configures position sharing
with the Heltec independently of receiver controls. See [GPS sharing](GPS_SHARING.md).
For reception checks, use [GPS diagnostics](GPS_DIAGNOSTICS.md).

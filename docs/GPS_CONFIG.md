# GPS configuration

Open **System > GPS > Config**. The CYD reads the NEO-6M through GPIO2 TX /
GPIO3 RX at 9600 baud; see [wiring and USB diagnostics](GPS_DIAGNOSTICS.md).
Opening Config starts a read if settings are missing. **Read receiver** refreshes
all settings without changing them. Startup sends no configuration commands.

Each dropdown initially says **Keep receiver**. The separate **Current** label
shows the last confirmed receiver value, or unknown. Select only the fields to
change, then **Apply**. Draft choices persist when moving between Config and Adv;
Apply on either page includes both pages. **Clear edits** discards the draft.

| Config setting | Choices |
| --- | --- |
| Update rate | 0.2, 0.5, 1, 2 or 5 Hz; 1 Hz recommended |
| Navigation mode | Portable, Stationary, Walking, Driving, At sea, Airborne <1g / <2g / <4g |
| SBAS assistance | Enabled or Disabled; preserves existing SBAS usage and scan masks |

Navigation modes describe motion assumptions in the receiver's position filter.
Walking maps to Pedestrian and Driving to Automotive in u-blox terminology.
SBAS availability depends on location, sky visibility and receiver support.

The **Adv** submenu contains:

| Adv setting | Choices and limits |
| --- | --- |
| Power mode | Maximum performance, Eco, Power save; Eco and Power save require 1 Hz in this UI |
| NMEA output | Normal or Compact; applies only to the connected GPS UART |

Adv also contains **Share GPS to Heltec** and **Use CYD GPS**. These control
CYD forwarding and Heltec configuration, separately from receiver Apply/Save.
See [GPS sharing and wiring migration](GPS_SHARING.md).

Normal requests GGA, GLL, GSA, GSV, RMC and VTG every navigation cycle. Compact
keeps GGA and RMC every cycle, disables redundant GLL/VTG, and requests GSA/GSV
at most once per five seconds. The latter reduces satellite-detail traffic;
it does not disable position updates. An existing receiver mix can show Custom.
Other NMEA or binary output configured outside these six messages is preserved.

Rates above 1 Hz require Compact output and Maximum performance. Select these
in Adv before applying the faster rate; the changes are sent together. The CYD
reduces output before raising the rate, and lowers the rate before restoring
Normal output. Baud remains 9600 because both boards share the GPS stream.
External message configurations can still consume bandwidth; monitor diagnostics.

Apply changes the receiver's running configuration. Each changed field needs
an ACK followed by matching readback. Unknown fields cannot be overwritten.
One operation runs at a time, with a two-second timeout per command/reply phase.
Unsupported settings may be rejected by a particular receiver or clone.
On failure, later changes stop; earlier confirmed changes remain applied. Read
the receiver before retrying. A batch is not an atomic transaction.

**Save** is separate from Apply and enabled only with a complete confirmed
snapshot and no pending draft. It requests storage of message, navigation and
receiver-management settings. The CYD does not save its own GPS defaults or
reapply settings at startup. A save ACK confirms acceptance, not retention after
power removal: storage support and backup power vary across NEO-6M modules.
Baud, port protocols, reset settings, configuration clear/load masks and other
ports' message rates are untouched by this feature.

The commands and payload masks follow the
[u-blox 6 receiver protocol specification](https://content.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_%28GPS.G6-SW-10018%29_Public.pdf),
including CFG-RATE, CFG-NAV5, CFG-SBAS, CFG-RXM, CFG-MSG and CFG-CFG.
Compile-time checks exercise the protocol and failure handling.

## v0.1.95 installation — October 23, 2026

The production CYD build was flashed through COM12 with esptool hash
verification. USB identity confirmed v0.1.95 in app0. A full `gps config` read
confirmed all ten fields (known mask 1023), with no configuration errors:
1000 ms measurement period, Portable navigation, SBAS enabled, Eco power and
Custom NMEA output. NMEA reception continued with zero checksum failures.
No receiver settings were changed or saved during verification. Touchscreen
Apply/Save and receiver persistence after power removal remain untested.

The health report confirmed a responsive Heltec link with zero decode errors,
available SD storage and MAX17048, the persisted battery discharge estimate,
and preserved 46% audio volume with Bubble Pop / Double Pop selections.

Artifact: `dist/s3-lora-interface-gps-config-v0.1.95-20261023.bin`
(1,852,064 bytes). SHA256:
`5d05c69fa7dc6e7e70a5a8fabae22ebfc9ab2cf9f7182e2c56d3644c99bd7014`.
Partition/settings backup: `build/pre-v0.1.95-flash-metadata.bin`.
Readback: `build/v0.1.95-gps-readback.json`. These generated artifacts and
captures remain local and ignored by Git.

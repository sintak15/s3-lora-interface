# Share CYD GPS with the Heltec

Firmware v0.1.96 can supply positions from the CYD's NEO-6M over the existing
115200-baud Protobuf UART. The Heltec's Meshtastic 2.7.15 application already
supports client-provided positions; no Heltec firmware update is required.

## Wiring migration

1. Install the CYD update with the existing wiring intact.
2. Open **System > GPS > Config > Adv**, then press **Use CYD GPS**.
3. Wait for sharing to become enabled. Setup reads a fresh Heltec configuration,
   changes its GPS mode to **Not present** and fixed position to off, then checks
   configuration readback before enabling the CYD preference. The deprecated
   GPS-enabled flag is also cleared. Broadcast policy, receiver pins and other
   position settings are preserved. The Heltec may reboot; setup resumes readback
   after reconnection. It allows up to 30 seconds for the asynchronous change,
   retrying reads without resending the configuration write. Failed setup leaves sharing off.
4. Confirm **Heltec accepted last update** with a fresh GPS fix. Queued and
   Accepted are separate counters. A queue acceptance confirms local ingestion,
   not a LoRa transmission or remote delivery.
5. Power off before removing only the branch from **GPS TX to Heltec GPIO5**.
   Keep GPS TX to CYD GPIO3, GPS RX to CYD GPIO2, GPS power and common ground.
   The CYD GPIO14/21 to Heltec GPIO39/38 link stays connected.

The switch defaults to off to preserve existing split-wire installations.
Turning it off stops forwarding without changing Heltec configuration. To
return to direct GPS reception, restore the GPIO5 branch and explicitly restore
the Heltec's original GPS settings. Switching off does not automatically do that.
Receiver Apply/Save controls are independent of sharing setup and preference.

## Forwarding behavior

The CYD sends only valid fixes at most six seconds old, no faster than once per
15 seconds. Checksum-valid no-fix RMC/GGA/GSA reports invalidate the candidate
immediately. Estimated/manual/simulated GGA positions are rejected. Invalid
coordinates and unknown or stale optional fields are not manufactured into a fix.
The reported fix type comes from GSA, rather than an estimate from satellite count.

Updates contain coordinates, available MSL altitude, UTC time, HDOP, fix quality,
GSA fix type, GSV satellites in view, speed and track. Missing fields are omitted
or left unspecified. Speed uses integer km/h and track uses degrees multiplied
by 100000, matching the installed Heltec 2.7.15 GPS parser rather than its stale
protobuf field comments. No-fix/stale samples, a stale UART link, unknown Heltec settings,
physical GPS still enabled, or fixed position selected block forwarding.

Packets target the Heltec's own confirmed node number on POSITION_APP, with no
ACK request, response request or direct mesh broadcast. Meshtastic applies its
existing position broadcast interval, movement rules and channel privacy settings.
The 15-second cadence respects the installed 2.7.15 API's ten-second position
rate limit. A phone also supplying position can compete with CYD updates; disable
phone location sharing when the CYD is intended to be the position source.

Queue results match the pending packet ID; unrelated or duplicate results do
not count as acceptance. Five seconds without a result marks a timeout. Failed
updates wait for the normal cadence and a newer fix. A lost link blocks sending
and requires Heltec configuration to be read again. Setup writes are user-initiated
and are never retried automatically at startup.

The CYD must remain powered and running. The Heltec can retain its last known
position when fresh input stops; stopping updates does not erase that position
or change its broadcast policy. Antenna reception remains necessary.

### Heltec screen limitation

The installed Heltec 2.7.15 screen reads satellite count and lock state from
its physical GPS status, not from client-provided position updates. With GPS
mode set to Not present, it displays **No GPS** even while CYD positions are
being accepted and used. Displaying the CYD's satellite count on that screen
requires a Heltec firmware change; CYD sharing alone cannot enable it. The
CYD's own GPS page continues to show satellites used and, when GSV is available,
satellites in view. These are different counts.

## Diagnostics and automation

USB `gps` and authenticated `/status` / `/diagnostics/report` include
`gpsShareEnabled`, `gpsShareState`, `gpsShareSetupBusy`, `gpsShareSetupError`,
`gpsShareQueued`, `gpsShareAccepted`, `gpsShareRejected`, `gpsShareTimeouts` and
`gpsShareWriteFailures`. These health fields contain no coordinates.

USB commands: `gps share setup`, `gps share on`, `gps share off`.
The authenticated **POST `/gps/share`** endpoint accepts a form field `action`
of `setup`, `on` or `off`. Setup returns 202 when queued; poll status for verified
completion. Invalid actions return 400, and busy/unavailable setup returns 409.
An accepted HTTP request is not confirmation that Heltec setup has completed.

## v0.1.96 installation — October 23, 2026

The production build and compile-time/browser regression checks passed. The
final application was installed through the authenticated Wi-Fi `/firmware`
endpoint; readback confirmed v0.1.96 in app0. The sharing preference survived
an OTA reboot. Existing audio selections/volume, SD access and the radio link
remained available.

A hardware check restored only the Heltec's original GPS-source fields, then
submitted one setup request. Setup completed, forwarding resumed through the
Heltec restart, and subsequent updates were accepted without another request.
This also exercised the wait/readback path for asynchronous admin changes.
Independent Heltec TCP readback reported LOC_EXTERNAL with a position about
1.3 seconds old. The final CYD capture showed nine satellites used, seven queued
and accepted updates, and zero rejected updates, queue timeouts, serial write
failures, NMEA checksum failures or radio decode errors.

Heltec firmware remained **2.7.15.567b8ea**. Only `position.gps_mode` differed
from the initial device-configuration snapshot; other device settings, module
settings and all eight channel slots matched the backup. Software checks cover
stale/invalid fixes and timer wrap. The working antenna was not unplugged during
hardware verification.
Physical removal of the GPIO5 branch remains a user wiring step.

Artifact: `dist/s3-lora-interface-gps-sharing-v0.1.96-20261023.bin`
(1,860,208 bytes). SHA256:
`73e75d621cf555b221717d97e250b4118d1bb065f8916b6bc2139c5e15adc5c4`.
Private configuration backups and verification captures are ignored under
`build/gps-sharing/`, including `final-migration-observation.json`,
`final-heltec-readback.json` and `v0.1.96-final-health.json`.

Sources for the installed Heltec version:
[external position handling](https://github.com/meshtastic/firmware/blob/567b8ea/src/modules/PositionModule.cpp),
[local routing](https://github.com/meshtastic/firmware/blob/567b8ea/src/mesh/Router.cpp),
[native GPS speed/track units](https://github.com/meshtastic/firmware/blob/567b8ea/src/gps/GPS.cpp),
[physical GPS screen status](https://github.com/meshtastic/firmware/blob/567b8ea/src/graphics/draw/UIRenderer.cpp),
[API position throttling](https://github.com/meshtastic/firmware/blob/567b8ea/src/mesh/PhoneAPI.cpp).

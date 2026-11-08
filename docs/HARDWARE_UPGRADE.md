# ATGM336H and ADS1115 installation checklist

Software preparation is complete; physical behavior is NOT yet bench verified.
No automatic flash or Heltec configuration changes have been performed.

The current build uses the [core wiring guide](WIRING.md), without outside,
light or 5V-rail sensors. The checklist below describes the optional expanded variant.

## Parts and wiring

- ATGM336H breakout with its supplied antenna.
- ADS1115 breakout; four 10 kohm 1% divider resistors and two 100 nF filter capacitors.
- TEMT6000 analog breakout powered from 3.3 V (bare sensor needs its own 10k load).
- Existing AM2302, a pullup if not built in, and 100 nF supply decoupling.
- The existing TP4056, MT3608, cell, switch, CYD and Heltec remain.
- No fuel gauge, compass or charging detection is installed.

Disconnect battery and all USB before changing wiring. Do not connect ADS1115 through a battery socket.
The CYD I2C connector supplies 3V3/GND/SDA16/SCL15 to ADS1115. ADDR -> GND;
A0 -> the 10k/10k divider on switched unboosted cell positive. Follow the
[complete battery wiring and power-off tests](BATTERY_STATS.md). Leave CYD
R14/R15/GPIO9 untouched and BAT socket empty. A1 senses boosted 5 V via a
second 10k/10k divider; A2 reads TEMT6000 SIG. See BATTERY_STATS.md for the
complete sensor wiring, placement and power-off checks. No CYD modifications are needed.

ATGM336H TX -> CYD GPIO3; GPS GND -> system ground. GPS RX and PPS
remain disconnected. GPIO2 is repurposed for AM2302 DATA, with GPS TX disabled
in firmware. Follow [AM2302 wiring and connector budget](AM2302.md); UART0 and
USB remain untouched. Verify the actual breakout supply rating and regulator
before choosing VCC: the bare receiver and a regulated carrier have different
limits. Do not assume the old GPS boosted-5V wire is suitable. UART signals must
be 3.3 V compatible. Attach the supplied antenna gently and place its ceramic
patch toward open sky, away from the boost, screen and LoRa antenna. Its physical
size/mounting may differ from the existing enclosure GPS holder.

## Bring-up when the modules arrive

1. Inspect labels/pinout and solder headers with all power disconnected.
2. Complete the meter checks in BATTERY_STATS.md before connecting A0.
3. Flash this branch with tools/build_ota_bin.ps1 -NoVersionBump after a successful
   verification build. The version has not been bumped for this hardware branch.
4. Probe I2C map: expect 0x48 plus existing 0x38 touch and 0x18 codec.
5. Compare filtered displayed cell volts with a meter after settling. The level
   should be marked approximate; no charging or runtime claim should appear.
6. Verify A1 reports meter-measured boosted voltage; cover/illuminate TEMT6000
   and confirm A2 relative signal changes. Brightness remains manual.
7. Outdoors with a clear view of sky, use USB `gps` / System > GPS. Confirm byte
   count and checksum-valid NMEA sentences increase, then obtain a fresh fix.
   First acquisition can take several minutes; do not judge reception indoors.
8. Confirm GP/GN sentence compatibility. If bytes arrive but no valid sentences,
   capture a short raw UART sample to confirm baud, talker IDs and checksums
   before changing the parser or sending receiver commands.
9. Verify position on the map. Configure/enable CYD GPS sharing separately only
   if desired, following GPS_SHARING.md. Do not connect GPS TX to multiple
   transmitter pins or connect Heltec TX to GPS RX.
10. Test missing ADS1115/unplugged sensing, low voltage availability and restart.
   Removing hardware/wires must only be done with power disconnected.
11. Check every power/USB combination for unintended backfeeding. Main switch
    off must not leave A0 powered while ADS1115 VDD is off.

ATGM336H configuration controls are intentionally unavailable pending verified
receiver command support. This does not prevent navigation or position sharing.
ADS1115 failures invalidate readings and retry once per second, without blocking
for the conversion. See GPS_CONFIG.md for the legacy u-blox distinction.

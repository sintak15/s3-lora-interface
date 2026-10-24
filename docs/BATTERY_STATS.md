# Battery statistics

The MAX17048 is the source of cell voltage, modeled state of charge (SOC) and
signed CRATE. The displayed percentage is rounded/clamped gauge SOC, without a
voltage correction or a software smoothing delay. The primary voltage is the
MAX VCELL reading. Firmware does not reset/quick-start the gauge to hide an
unexpected reading. The retired `batTrim10` preference is removed at startup.

**Battery trend** describes the sign of the current CRATE register: increasing,
decreasing, steady (zero), or unavailable after a failed read. There is no software settling
window, voltage-step override or retained charging flag. This is an averaged
SOC trend, not instantaneous current direction or charger presence. The chip
cannot directly detect the separate 5 V charger's connection; an actual charger
status input or suitable current sensor is needed for that measurement. CYD USB
powers the interface but does not charge the main cell in this build.

A separate filtered voltage remains available for diagnostics and qualified
low-voltage warnings. Thresholds remain 20% low and 10% critical, or qualified
voltage at/below 3.65 V and 3.50 V for 30 seconds. These UI warnings do not
replace battery protection or charge control.

## Remaining time

Runtime is an approximation calculated in firmware from fresh gauge SOC and a
learned discharge rate in percentage points per hour. No powered-off interval
is counted as active use, and a saved remaining-minutes value is never restored.

The touchscreen and web UI show the basis of each estimate:

- **Previous use:** a retained rate, including one restored from ESP32 NVS at boot.
- **Gauge trend:** two consistent, valid negative CRATE readings at least a minute
  apart. This permits an early estimate on a device without saved history.
- **Current use:** measured raw SOC decline of at least 0.5 percentage points over
  at least three minutes, using a rolling window of up to 15 minutes.

New qualified rates blend 25% into the existing learned rate once per minute.
A rising MAX SOC trend, failed reads, SOC rises and long sample gaps clear
session observations or pause the estimate, while retaining the learned rate.
Time is recomputed from current SOC on every valid sample and displayed rounded
to five minutes. With no saved rate, a flat SOC and unusable CRATE, the display
continues to say it is estimating instead of inventing a runtime.

The first qualified rate is saved promptly as `batRateV1`. Subsequent saves need
a change of at least 0.5 percentage points/hour or 5%, whichever is greater, and
are limited to once per 15 minutes, including after restarting with a saved rate.
Failed first-save attempts retry no faster than once a minute. This works with
a physical power switch and avoids continuous flash writes. A battery replacement
or different screen/Wi-Fi load may make a prior-use estimate inaccurate until new
observations adapt it.

## Continuous battery and power health logs

Diagnostics run automatically whenever the device is powered. No Start, Stop,
Charge, Discharge or Rest buttons are needed. The old manual notes did not
measure charger status and are retired. Gauge readings and error counters are
sampled continuously; a readable record and detailed CSV row are flushed every
10 seconds. The battery direction comes only from MAX CRATE.

On the CYD, use **System > Battery > View logs**. **Current log** and **Previous
log** select the file; **Refresh log** loads recent entries without interrupting
recording. Scroll within the log to read it. The device shows the latest complete
entries from a bounded 4 KiB tail, while web downloads provide the complete file.
The viewer refreshes on request so incoming samples do not move the text while
it is being read. No Wi-Fi connection is needed for on-device viewing.

The CYD **48h rotation** checkbox and web **Rotate logs every 48 hours of use**
setting control retention, not measurement. By default, each 48 hours of powered-on
recording archives the current readable/CSV pair and starts a fresh pair. One
previous pair is retained; the older previous pair is removed at the next
rotation. This retains roughly 48–96 hours of recording, depending on the age
of the current file. The disabled setting keeps appending to the current files
without a time or 16 MiB size stop, until the card is full. Re-enabling rotation
rotates an over-age current file on the next recording attempt. The previously
saved archive remains downloadable while rotation is disabled.

The setting persists in NVS. Recording age is recovered from the last flushed
readable record across restarts, including repeated short power cycles, with a
15-minute NVS checkpoint as fallback. A hard power loss can lose the latest
record and up to roughly 10 seconds of age. Powered-off time is not counted.
A persisted rotation journal and a directory rename keep both files together;
interrupted rotation is completed safely at the next startup/attempt. Downloads
and busy SD access defer recording/rotation. Sampled voltage statistics are
retained until the next successful record. Actual write failures and SD-busy/
unavailable delays have separate counters, all counting the current boot.

Files are under `/s3-lora/power-health/current/` and
`/s3-lora/power-health/previous/`, named `power-health.log` and
`battery-cycle.csv`. Existing v0.1.100 files are migrated from `/s3-lora/`
without overwriting newer files. Rotation deletes only the two owned files
in the previous directory; unexpected additional files stop rotation safely.

Readable records explain the following with units:

- GPS UTC when fresh, or explicitly unavailable; session, boot, uptime and
  recording age remain useful without GPS.
- Battery voltage, percentage, **Battery: +**, **Battery: -**, **Battery: =**
  or **Battery: Unavailable**, and percentage-point change per hour.
- Valid sample count, min/max, span, standard deviation and largest observed
  step from the normal 1 Hz voltage sampler.
- MAX read attempts/failures, UART decode/framing errors, codec/output errors
  and recoveries, Wi-Fi signal strength, radio response age and SD errors.

The optional CSV additionally includes raw VCELL/SOC/CRATE and read-only
STATUS/CONFIG register snapshots with validity flags. Its legacy `phase` column
contains `automatic` for new records. Error counters reset at each boot; compare
changes within the same boot. Zero Wi-Fi RSSI means unavailable or AP mode.
No coordinates, chat bodies or network credentials are included.

Variation includes normal load/charging changes and gauge updates. It is not a
measurement of RF noise or high-frequency switching ripple. Brief supply dips
can be missed. There is no boosted-5 V or Heltec-3.3 V sensing input, current
sensor or audio-quality measurement in this diagnostic path. Failed sensor reads
are marked invalid; retained numbers are stale. Normal use, a manual battery
cycle, or a hardware change can all be reviewed through these same ordinary logs.

## API semantics

`batteryGaugeTrend` and `batteryStateSource` identify the MAX CRATE source;
`batteryChargerConnected` is `null` because that hardware measurement is absent.
Legacy `batteryCharging` means only **valid positive MAX CRATE**; it must not be
interpreted as physical charger status. `batteryChargeRateValid` distinguishes
an unreadable rate from zero. Legacy corrected-SOC fields mirror the gauge;
all calibration offsets are zero. `voltage` is raw VCELL and `senseVoltage` is
the separate filtered diagnostic voltage. `batteryRuntimeMinutes` remains a
software estimate; a rising trend pauses it while preserving the learned rate.

Authenticated `POST /battery/cycle` accepts `action=rotation-on` or
`rotation-off`; removed manual start/stop/phase actions return 410. Unknown
actions return 400. Settings that cannot be saved return 409; a lost response
has an unknown outcome, so read status before retrying.

Download the current logs with `GET /sd/power-health` and
`GET /sd/battery-cycle`; append `/previous` for their previous files.
`batteryCycleEnabled` is always true and the compatibility `batteryCyclePhase`
is `automatic`. `batteryLogRotationEnabled`, `batteryLogWindowSeconds` and
`batteryLogPreviousAvailable` describe retention. Existing status/error/voltage
statistics remain available in status and field reports.

v0.1.98 briefly introduced a voltage/SOC history classifier, but its charging
confirmation was too conservative during a real charge. v0.1.99 removes it in
favor of the gauge-only readings described here.

Reference: [Analog Devices MAX17048/MAX17049 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
especially ModelGauge theory, voltage versus SOC, temperature compensation,
quick-start and CRATE.

From v0.1.101, the +, - and = glyphs are bold on the CYD and web UI; the
symbols mean increasing, decreasing and steady gauge percentage. They do not
assert physical charger presence. Detailed API/CSV fields retain their technical
SOC/CRATE names for analysis.

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

## Battery and power health recorder

Use **System > Battery** on the CYD, or **Tools > Health & diagnostics > Battery
& runtime** on the web UI. Start logging, then mark **charging**, **discharging**
or **resting** as you physically change the separate charger connection. These
markers are operator observations and never change the MAX readings or runtime
learning. Stop logging when finished; download the CSV from the web page.

A useful sequence is charge normally to the charger's full indication, mark a
rest, disconnect the charger and mark discharging, then use the device normally
to its usual low-battery warning. Reconnect and mark charging. There is no need
to force a protection cutoff or deeply discharge the cell. The firmware cannot
switch the charger/load, validate physical full/empty, or measure mAh capacity.
No multi-hour physical cycle was performed as part of the software checks.

The main download is the append-only `/s3-lora/power-health.log`: plain-language
entries with units, gauge validity, user-marked charger phase, sampled voltage
variation, connection/audio errors and radio response age. UTC comes from fresh
GPS date/time when available; otherwise it explicitly says unavailable. Session,
boot identifiers and uptime remain usable without a GPS fix.

The optional `/s3-lora/battery-cycle.csv` contains the same observations in
columns for more detailed analysis, plus raw gauge registers:


- Session and boot identifiers, uptime, firmware version and start/resume/stop/phase events.
- Raw VCELL, SOC, signed CRATE (raw registers in hex), decoded voltage/SOC/rate,
  read validity, and read-only STATUS/CONFIG snapshots with their validity flags.
- Minimum/maximum cell voltage, span, standard deviation and largest sampled step
  from the normal 1 Hz sampler between rows,
  estimated remaining minutes, and radio response age. Millisecond RF supply
  dips can be missed; this recorder does not replace an oscilloscope.
- MAX register read attempts/errors, UART frame/decode/timeout/length counters,
  codec I/O and audio-output write errors/recoveries, Wi-Fi RSSI and SD errors.
  Error counters are cumulative **since that boot**, not errors per record.
  Compare counter differences within one boot. Zero Wi-Fi RSSI means unavailable
  or AP mode. Signal strength does not directly measure RF noise.

Voltage variation includes ordinary load changes, charging slope and gauge ADC
updates. It does not establish EMI, electrical ripple at switching frequencies,
or audible distortion. A failed read is logged as invalid; retained numerical
values are stale and must not be interpreted as fresh measurements. Hardware
has no ADC connection for the boosted 5 V or Heltec 3.3 V rail, RF-noise detector,
current sensor or microphone. These cannot be measured by this firmware.
No special before/after mode is needed: keep ordinary logs while changing one
thing at a time, using comparable battery SOC, display/Wi-Fi/radio settings and
load, and inspect timestamps/errors around the observed symptom.

Samples flush every 10 seconds, and each manual phase mark writes immediately.
Logging enable, session and phase are stored in NVS only on operator actions,
so a restart resumes the same session with a new boot identifier and `resume`
row. Uptime resets per boot; powered-off time is not a discharge observation.
Rows/errors in the UI count the current boot. From v0.1.100, busy/unavailable
SD attempts are reported as recording delays separately from actual log-write
failures. The logger retries at most once a second and retains sampled voltage
statistics until a successful record. It does not invent the missing individual
SOC/rate records. v0.1.99 combined these delays and failures in its error count. A power loss can
still lose or partially write the final record. Keep the original CSV when
reviewing incomplete records. Repeated Start while running is idempotent.
Logging stops if either file reaches 16 MiB; earlier tests are never silently overwritten.
Download/archive them and remove the archived log files from the SD card
manually before another test if the limit is reached. No location, chat body or network credentials are logged.

## API semantics

`batteryGaugeTrend` and `batteryStateSource` identify the MAX CRATE source;
`batteryChargerConnected` is `null` because that hardware measurement is absent.
Legacy `batteryCharging` means only **valid positive MAX CRATE**; it must not be
interpreted as physical charger status. `batteryChargeRateValid` distinguishes
an unreadable rate from zero. Legacy corrected-SOC fields mirror the gauge;
all calibration offsets are zero. `voltage` is raw VCELL and `senseVoltage` is
the separate filtered diagnostic voltage. `batteryRuntimeMinutes` remains a
software estimate; a rising trend pauses it while preserving the learned rate.

Cycle controls use authenticated `POST /battery/cycle` with form `action=start`,
`stop`, `charging`, `discharging` or `resting`. Invalid actions return 400 and
unrecordable actions return 409. The authenticated `GET /sd/battery-cycle`
downloads the CSV; `GET /sd/power-health` downloads the readable log.
`batteryCycleEnabled`, `batteryCyclePhase`,
`batteryCycleStatus`, `batteryCycleRows` and `batteryCycleErrors` appear in status
and field reports. A lost HTTP response leaves the operation's result unknown;
read status before retrying.

v0.1.98 briefly introduced a voltage/SOC history classifier, but its charging
confirmation was too conservative during a real charge. v0.1.99 removes it in
favor of the gauge-only readings described here.

Reference: [Analog Devices MAX17048/MAX17049 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
especially ModelGauge theory, voltage versus SOC, temperature compensation,
quick-start and CRATE.

From v0.1.100, user-facing state wording is **Battery: Increasing**, **Battery:
Decreasing**, **Battery: Steady**, or **Battery: Unavailable**. **Level change**
shows the rate per hour. Detailed CSV/API fields retain SOC/CRATE register names
for technical analysis. This wording does not infer physical charger status.

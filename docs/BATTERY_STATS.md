# Battery statistics (v0.1.92)

The MAX17048 owns battery state of charge. Firmware rounds and gently smooths
that percentage for display; it does not add a voltage-derived percentage trim.
The old `batTrim10` setting is removed on startup. A low SOC reading is no longer
overridden or used to trigger quick-start just because cell voltage looks high.
Cell voltage under load is not a unique indication of state of charge.

Voltage remains available for diagnostics and sustained low-voltage warnings.
Percentage warnings use the gauge directly, so display smoothing cannot delay
them. Existing thresholds remain 20% low and 10% critical, or qualified voltage
at/below 3.65 V and 3.50 V respectively for 30 seconds. These are UI warnings,
not a battery protection or charger controller.

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
Charging, noisy voltage, failed reads, SOC rises and long sample gaps clear
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

## Interpretation and diagnostics

The MAX17048 provides voltage, modeled SOC and an approximate SOC change rate;
CRATE is not a direct current measurement. Runtime depends on future usage
resembling recent usage and does not account precisely for the power assembly's
cutoff voltage. Charging is inferred from gauge rate and voltage behavior; USB
presence is not a charger-status signal. The CYD USB connection does not charge
the cell in this build.

No battery temperature sensor is currently used for RCOMP compensation. The
gauge's default model may not perfectly match the installed cell. Manufacturer
supported model configuration and measured battery-temperature compensation are
the appropriate ways to improve gauge accuracy, rather than applying a generic
voltage-to-percentage offset.

Status and field reports include `batteryRuntimeBasis`,
`batteryLearnedDischargePercentHr` and `batterySavedDischargePercentHr`, alongside
SOC, raw and filtered voltage, and CRATE. Legacy corrected-SOC fields mirror the
gauge and legacy calibration offsets remain zero; no correction is applied.

The estimator regressions cover reboot/charge recovery, fresh SOC, early gauge
qualification, noisy/flat/rising readings, changed load, timer wrap, rounding
and bounded persistence writes. Browser tests cover all runtime display states.
They do not replace a measured battery discharge test.

Reference: [Analog Devices MAX17048/MAX17049 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
especially ModelGauge theory, voltage versus SOC, temperature compensation,
quick-start and CRATE.

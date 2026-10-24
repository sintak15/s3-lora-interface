// User-operated cycle recorder. Never switches a charger/load or rewrites MAX SOC.
static const char* batteryCyclePhase() {
  static const char* phases[] = {"unmarked", "charging", "discharging", "resting"};
  return phases[batteryCycle.phase < 4 ? batteryCycle.phase : 0];
}

static bool writeBatteryCycleRow(const char* event) {
  if (!sdStorage.available) {
    batteryCycle.unavailableDeferrals++;
    strlcpy(batteryCycle.status, "Waiting for SD; recording deferred", sizeof(batteryCycle.status));
    return false;
  }
  // Do not delay radio/UI work behind a download or map read.
  SdLock lock(0);
  if (!lock) {
    batteryCycle.busyDeferrals++;
    strlcpy(batteryCycle.status, "SD busy; recording deferred", sizeof(batteryCycle.status));
    return false;
  }
  File file = SD_MMC.open(SD_BATTERY_CYCLE_PATH, FILE_APPEND);
  File readable = SD_MMC.open(SD_POWER_HEALTH_PATH, FILE_APPEND);
  if (!file || !readable) {
    if (file) file.close();
    if (readable) readable.close();
    batteryCycle.errors++;
    strlcpy(batteryCycle.status, "SD open failed", sizeof(batteryCycle.status));
    return false;
  }
  // Append across sessions and restarts; never silently erase an earlier test.
  if (file.size() >= 16UL * 1024 * 1024 || readable.size() >= 16UL * 1024 * 1024) {
    file.close();
    readable.close();
    batteryCycle.enabled = false;
    prefs.putBool("batCycleOn", false);
    strlcpy(batteryCycle.status, "Stopped: 16 MiB log limit", sizeof(batteryCycle.status));
    return false;
  }
  static const char header[] = "session,boot,uptime_ms,event,phase,gauge_valid,crate_valid,vcell_raw,soc_raw,crate_raw,status_valid,status_raw,config_valid,config_raw,soc_percent,cell_mv,soc_rate_percent_hr,cell_min_mv,cell_max_mv,runtime_minutes,radio_seen,radio_age_ms,firmware,voltage_samples,voltage_span_mv,voltage_stddev_mv,largest_step_mv,max_read_attempts,max_read_errors,uart_frames,uart_decode_errors,uart_timeouts,uart_bad_lengths,codec_io_errors,audio_write_errors,audio_recoveries,wifi_rssi_dbm,sd_write_errors,log_errors,utc\n";
  bool ok = file.size() != 0 || file.print(header) == sizeof(header) - 1;
  uint16_t status = 0, config = 0;
  bool statusValid = max17048ReadReg16(0x1A, status);
  bool configValid = max17048ReadReg16(0x0C, config);
  batteryCycle.lastSpanMv = batteryCycle.voltageWindow.span();
  batteryCycle.lastStdDevMv = sqrt(batteryCycle.voltageWindow.variance());
  batteryCycle.lastStepMv = batteryCycle.voltageWindow.largestStep();
  batteryCycle.lastSamples = batteryCycle.voltageWindow.count();
  char utc[32] = "unavailable";
  if (localGps.date.isValid() && localGps.time.isValid() && localGps.date.age() < 30000 && localGps.time.age() < 30000) {
    snprintf(utc, sizeof(utc), "%04u-%02u-%02uT%02u:%02u:%02uZ", (unsigned)localGps.date.year(),
             (unsigned)localGps.date.month(), (unsigned)localGps.date.day(), (unsigned)localGps.time.hour(),
             (unsigned)localGps.time.minute(), (unsigned)localGps.time.second());
  }
  char row[640];
  int length = snprintf(row, sizeof(row),
    "%08lx,%08lx,%lu,%s,%s,%u,%u,%04X,%04X,%04X,%u,%04X,%u,%04X,%.3f,%lu,%.3f,%lu,%lu,%ld,%u,%lu,%s,%lu,%lu,%.3f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%ld,%lu,%lu,%s\n",
    (unsigned long)batteryCycle.session, (unsigned long)batteryCycle.boot, (unsigned long)millis(),
    event, batteryCyclePhase(), localBattery.gaugePresent, localBattery.chargeRateValid,
    localBattery.rawVcell, localBattery.rawGaugeSoc, localBattery.rawCrate,
    statusValid, status, configValid, config, localBattery.gaugeSoc,
    (unsigned long)localBattery.rawPackMv, localBattery.chargeRatePercentHr,
    (unsigned long)batteryCycle.voltageWindow.minimum(),
    (unsigned long)batteryCycle.voltageWindow.maximum(), (long)batteryRuntime.minutes(), radioResponseSeen,
    (unsigned long)(radioResponseSeen ? millis() - lastRadioResponseMs : 0), FIRMWARE_VERSION,
    (unsigned long)batteryCycle.lastSamples, (unsigned long)batteryCycle.lastSpanMv,
    batteryCycle.lastStdDevMv, (unsigned long)batteryCycle.lastStepMv,
    (unsigned long)localBattery.readAttempts, (unsigned long)localBattery.readErrors,
    (unsigned long)framesDecoded, (unsigned long)decodeErrors, (unsigned long)serialFrameTimeouts,
    (unsigned long)invalidFrameLengths, (unsigned long)toneCodecIoFailures, (unsigned long)toneWriteFailures,
    (unsigned long)toneRecoveries, (long)(wifiEnabled && !wifiApMode && WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0),
    (unsigned long)sdStorage.writeErrors, (unsigned long)batteryCycle.errors, utc);
  ok = ok && length > 0 && (size_t)length < sizeof(row) && file.print(row) == (size_t)length;
  file.close(); // Flush each row; a hard power cut can still lose the last SD write.
  char entry[1600];
  int entryLength = snprintf(entry, sizeof(entry),
    "\nUTC %s | Uptime %lu s | session %08lx | boot %08lx | %s | firmware %s\n"
    "  Charger phase marked by you: %s.\n"
    "  Battery sensor: %s; level %.2f%%; cell %.3f V; Battery: %s (change %.2f percentage points/hour; rate %s).\n"
    "  Voltage since last record: %lu valid 1 Hz samples; min/max %.3f/%.3f V; span %lu mV; standard deviation %.2f mV; largest sampled step %lu mV.\n"
    "  Errors since this boot: MAX reads %lu of %lu; UART decode %lu, timeout %lu, invalid length %lu; audio codec %lu, output writes %lu; audio recoveries %lu.\n"
    "  Radio response seen: %s; last response %lu s ago. Wi-Fi signal: %ld dBm (0 = unavailable/AP mode).\n"
    "  Battery warning: %s. Runtime estimate: %ld minutes (-1 = unavailable). SD write errors: %lu; failed log writes: %lu.\n"
    "  Recording delays: SD busy %lu attempts; SD unavailable %lu attempts. Voltage samples are retained until the next successful record.\n",
    utc, (unsigned long)(millis() / 1000), (unsigned long)batteryCycle.session, (unsigned long)batteryCycle.boot, event, FIRMWARE_VERSION,
    batteryCyclePhase(), localBattery.gaugePresent ? "available" : "READ FAILED; battery values stale",
    localBattery.gaugeSoc, localBattery.rawPackMv / 1000.0f,
    Max17048::trendLabel(localBattery.gaugePresent && localBattery.chargeRateValid, localBattery.chargeRatePercentHr),
    localBattery.chargeRatePercentHr, localBattery.chargeRateValid ? "valid" : "unavailable; rate value stale",
    (unsigned long)batteryCycle.lastSamples, batteryCycle.voltageWindow.minimum() / 1000.0f,
    batteryCycle.voltageWindow.maximum() / 1000.0f, (unsigned long)batteryCycle.lastSpanMv,
    batteryCycle.lastStdDevMv, (unsigned long)batteryCycle.lastStepMv,
    (unsigned long)localBattery.readErrors, (unsigned long)localBattery.readAttempts,
    (unsigned long)decodeErrors, (unsigned long)serialFrameTimeouts, (unsigned long)invalidFrameLengths,
    (unsigned long)toneCodecIoFailures, (unsigned long)toneWriteFailures, (unsigned long)toneRecoveries,
    radioResponseSeen ? "yes" : "no", (unsigned long)(radioResponseSeen ? (millis() - lastRadioResponseMs) / 1000 : 0),
    (long)(wifiEnabled && !wifiApMode && WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0),
    localBattery.gaugePresent ? batteryAlertLabel() : "unknown", (long)batteryRuntime.minutes(),
    (unsigned long)sdStorage.writeErrors, (unsigned long)batteryCycle.errors,
    (unsigned long)batteryCycle.busyDeferrals, (unsigned long)batteryCycle.unavailableDeferrals);
  bool readableOk = entryLength > 0 && (size_t)entryLength < sizeof(entry) && readable.print(entry) == (size_t)entryLength;
  readable.close();
  ok = ok && readableOk;
  if (ok) {
    batteryCycle.rows++;
    sdStorage.writes++;
    batteryCycle.voltageWindow.resetWindow();
    strlcpy(batteryCycle.status, batteryCycle.enabled ? "Recording to SD" : "Stopped; CSV retained", sizeof(batteryCycle.status));
  } else {
    batteryCycle.errors++;
    sdStorage.writeErrors++;
    strlcpy(batteryCycle.status, "SD write failed", sizeof(batteryCycle.status));
  }
  return ok;
}

static bool controlBatteryCycle(const String& action) {
  if (action == "start") {
    if (batteryCycle.enabled) return true; // Repeated taps do not restart the session.
    if (!sdStorage.available) return false;
    batteryCycle.session = esp_random();
    batteryCycle.phase = 0;
    batteryCycle.voltageWindow = SampleVariation();
    if (prefs.putUInt("batCycleId", batteryCycle.session) != sizeof(uint32_t) ||
        prefs.putUChar("batCyclePhase", 0) != sizeof(uint8_t) ||
        prefs.putBool("batCycleOn", true) != sizeof(bool)) return false;
    batteryCycle.enabled = true;
    batteryCycle.bootPending = false;
    batteryCycle.lastWriteMs = millis();
    if (writeBatteryCycleRow("start")) return true;
    batteryCycle.enabled = false;
    prefs.putBool("batCycleOn", false);
    return false;
  }
  if (!batteryCycle.enabled) return action == "stop";
  if (action == "stop") {
    if (prefs.putBool("batCycleOn", false) != sizeof(bool)) return false;
    batteryCycle.enabled = false;
    return writeBatteryCycleRow("stop");
  }
  uint8_t phase = action == "charging" ? 1 : action == "discharging" ? 2 : action == "resting" ? 3 : 0;
  if (!phase || prefs.putUChar("batCyclePhase", phase) != sizeof(uint8_t)) return false;
  batteryCycle.phase = phase;
  batteryCycle.lastWriteMs = millis();
  return writeBatteryCycleRow("marker");
}

static void sampleBatteryCycle() {
  if (!batteryCycle.enabled) return;
  if (localBattery.gaugePresent && localBattery.lastGaugeMs != batteryCycle.observedGaugeMs) {
    batteryCycle.observedGaugeMs = localBattery.lastGaugeMs;
    batteryCycle.voltageWindow.add(localBattery.rawPackMv);
  }
  uint32_t now = millis();
  if (batteryCycle.lastAttemptMs && (uint32_t)(now - batteryCycle.lastAttemptMs) < 1000) return;
  if (!batteryCycle.bootPending && (uint32_t)(now - batteryCycle.lastWriteMs) < 10000) return;
  batteryCycle.lastAttemptMs = now;
  if (writeBatteryCycleRow(batteryCycle.bootPending ? "resume" : "sample")) {
    batteryCycle.bootPending = false;
    batteryCycle.lastWriteMs = now;
  }
}

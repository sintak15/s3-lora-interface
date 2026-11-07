// Continuous power diagnostics; battery voltage comes from the ADC.
// Legacy CSV phase column now identifies automatic recording.
static const char* batteryCyclePhase() { return "automatic"; }

static bool writeBatteryCycleRow(const char* event) {
  if (!sdStorage.available) {
    batteryCycle.unavailableDeferrals++;
    strlcpy(batteryCycle.status, "Waiting for SD; recording deferred", sizeof(batteryCycle.status));
    return false;
  }
  if (powerLogDownloadActive()) {
    batteryCycle.busyDeferrals++;
    strlcpy(batteryCycle.status, "SD transfer active; recording deferred", sizeof(batteryCycle.status));
    return false;
  }
  // Do not delay radio/UI work behind a download or map read.
  SdLock lock(0);
  if (!lock) {
    batteryCycle.busyDeferrals++;
    strlcpy(batteryCycle.status, "SD busy; recording deferred", sizeof(batteryCycle.status));
    return false;
  }
  if (!batteryCycle.pathsReady && !preparePowerLogStorage()) {
    batteryCycle.errors++;
    strlcpy(batteryCycle.status, "Could not prepare log storage", sizeof(batteryCycle.status));
    return false;
  }
  bool rotated = batteryCycle.rotationPending || batteryCycle.rotationClock.due(batteryCycle.rotationEnabled);
  if (rotated) {
    batteryCycle.pathsReady = false;
    PowerLogStorage storage;
    if (!PowerLog::rotate(storage)) {
      batteryCycle.errors++;
      strlcpy(batteryCycle.status, "Rotation pending; retrying safely", sizeof(batteryCycle.status));
      return false;
    }
    batteryCycle.previousAvailable = sdCardFilesystem().exists(SD_POWER_HEALTH_PREVIOUS_PATH);
    batteryCycle.pathsReady = true;
  }
  File file = sdCardFilesystem().open(SD_BATTERY_CYCLE_PATH, FILE_APPEND);
  File readable = sdCardFilesystem().open(SD_POWER_HEALTH_PATH, FILE_APPEND);
  if (!file || !readable) {
    if (file) file.close();
    if (readable) readable.close();
    batteryCycle.errors++;
    strlcpy(batteryCycle.status, "SD open failed; retrying", sizeof(batteryCycle.status));
    return false;
  }
  if (rotated) event = "rotation";
  static const char header[] = "session,boot,uptime_ms,event,phase,gauge_valid,crate_valid,vcell_raw,soc_raw,crate_raw,status_valid,status_raw,config_valid,config_raw,soc_percent,cell_mv,soc_rate_percent_hr,cell_min_mv,cell_max_mv,runtime_minutes,radio_seen,radio_age_ms,firmware,voltage_samples,voltage_span_mv,voltage_stddev_mv,largest_step_mv,max_read_attempts,max_read_errors,uart_frames,uart_decode_errors,uart_timeouts,uart_bad_lengths,codec_io_errors,audio_write_errors,audio_recoveries,wifi_rssi_dbm,sd_write_errors,log_errors,utc\n";
  bool ok = file.size() != 0 || file.print(header) == sizeof(header) - 1;
  uint16_t status = 0, config = 0;
  bool statusValid = false;
  bool configValid = false;
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
    event, batteryCyclePhase(), false, false,
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
  char battery[80], variation[48], step[32];
  if (localBattery.voltageValid) {
    snprintf(battery, sizeof(battery), "%.3f V, ~%.1f%% (%s)", localBattery.rawPackMv / 1000.0f,
             localBattery.gaugeSoc, batteryChargingStatus);
  } else strlcpy(battery, "unavailable", sizeof(battery));
  if (batteryCycle.lastSamples >= 2) {
    snprintf(variation, sizeof(variation), "%lu mV", (unsigned long)batteryCycle.lastSpanMv);
    snprintf(step, sizeof(step), "%lu mV", (unsigned long)batteryCycle.lastStepMv);
  } else {
    strlcpy(variation, "waiting for samples", sizeof(variation));
    strlcpy(step, "unavailable", sizeof(step));
  }
  char entry[384];
  int entryLength = snprintf(entry, sizeof(entry),
    "\nUTC %s | Battery: %s | Variation: %s | Step: %s | Log age: %llu s\n",
    utc, battery, variation, step, (unsigned long long)(batteryCycle.rotationClock.elapsed() / 1000));
  bool readableOk = entryLength > 0 && (size_t)entryLength < sizeof(entry) && readable.print(entry) == (size_t)entryLength;
  char boost[24], light[48];
  if (auxiliaryAnalog.boostValid) snprintf(boost, sizeof(boost), "%.3f V", auxiliaryAnalog.boostMv / 1000.0f);
  else strlcpy(boost, "unavailable", sizeof(boost));
  if (auxiliaryAnalog.lightValid) snprintf(light, sizeof(light), "%u%% relative, %.3f V",
      (unsigned)auxiliaryAnalog.lightPercent, auxiliaryAnalog.lightMv / 1000.0f);
  else strlcpy(light, "unavailable", sizeof(light));
  int analogLength = snprintf(entry, sizeof(entry), "  Boost A1: %s | Light A2: %s\n", boost, light);
  readableOk = readableOk && analogLength > 0 && (size_t)analogLength < sizeof(entry) && readable.print(entry) == (size_t)analogLength;
  if (environmentAvailable()) {
    int count = snprintf(entry, sizeof(entry), "  Outside: %.1f C | Humidity: %.1f%% RH\n",
        environmentReading.temperatureTenths / 10.0f, environmentReading.humidityTenths / 10.0f);
    readableOk = readableOk && count > 0 && (size_t)count < sizeof(entry) && readable.print(entry) == (size_t)count;
  } else readableOk = readableOk && readable.print("  Outside AM2302: unavailable\n") > 0;
  if (batteryCycle.bootPending || rotated) {
    int count = snprintf(entry, sizeof(entry),
      "  %s | %s | boot %08lx | Rotation: %s. Variation and step use 1 Hz samples; fast spikes may be missed.\n",
      rotated ? "New log" : "Restart", FIRMWARE_VERSION, (unsigned long)batteryCycle.boot,
      batteryCycle.rotationEnabled ? "48 hours of use" : "unlimited");
    readableOk = readableOk && count > 0 && (size_t)count < sizeof(entry) && readable.print(entry) == (size_t)count;
  }
  const uint32_t issues[] = {localBattery.readErrors, decodeErrors, serialFrameTimeouts, invalidFrameLengths,
                             toneCodecIoFailures, toneWriteFailures, toneRecoveries, sdStorage.writeErrors, batteryCycle.errors};
  const char* labels[] = {"battery read failures", "radio decode errors", "radio timeouts", "radio framing errors",
                          "audio codec errors", "audio output errors", "audio recoveries", "SD write errors", "log write errors"};
  for (unsigned i = 0; i < 9; ++i) {
    uint32_t added = issues[i] - batteryCycle.reportedIssues[i];
    if (!added) continue;
    int count = snprintf(entry, sizeof(entry), "  %s: +%lu since last record\n", labels[i], (unsigned long)added);
    readableOk = readableOk && count > 0 && (size_t)count < sizeof(entry) && readable.print(entry) == (size_t)count;
  }
  readable.close();
  ok = ok && readableOk;
  if (ok) {
    memcpy(batteryCycle.reportedIssues, issues, sizeof(issues));
    batteryCycle.rows++;
    sdStorage.writes++;
    batteryCycle.voltageWindow.resetWindow();
    strlcpy(batteryCycle.status, "Continuous recording to SD", sizeof(batteryCycle.status));
  } else {
    batteryCycle.errors++;
    sdStorage.writeErrors++;
    strlcpy(batteryCycle.status, "SD write failed", sizeof(batteryCycle.status));
  }
  return ok;
}

static bool controlBatteryCycle(const String& action) {
  if (action == "rotation-on" || action == "rotation-off") {
    return setPowerLogRotation(action == "rotation-on");
  }
  return false;
}

static void sampleBatteryCycle() {
  batteryCycle.rotationClock.tick(millis());
  if (millis() - batteryCycle.lastCheckpointMs >= PowerLog::CHECKPOINT_MS) {
    batteryCycle.lastCheckpointMs = millis();
    if (prefs.putULong64("batLogAge", batteryCycle.rotationClock.elapsed()) != sizeof(uint64_t)) {
      batteryCycle.errors++;
      strlcpy(batteryCycle.status, "Rotation clock save failed", sizeof(batteryCycle.status));
    }
  }
  if (localBattery.voltageValid && localBattery.lastGaugeMs != batteryCycle.observedGaugeMs) {
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

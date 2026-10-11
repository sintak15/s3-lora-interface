// Internal implementation module; included once by the main sketch.
static unsigned long bytesToWholeKb(uint64_t bytes) {
  return (unsigned long)((bytes + 1023ULL) / 1024ULL);
}

static unsigned long bytesToWholeMb(uint64_t bytes) {
  return (unsigned long)((bytes + (1024ULL * 1024ULL - 1ULL)) / (1024ULL * 1024ULL));
}

static unsigned long heapCapKb(uint32_t caps) {
  return bytesToWholeKb(heap_caps_get_free_size(caps));
}

static unsigned long heapCapLargestKb(uint32_t caps) {
  return bytesToWholeKb(heap_caps_get_largest_free_block(caps));
}

static void recordMapRenderTimingDuration(uint32_t durationMs, bool drewTiles, bool cacheHit) {
  lastMapRenderDurationMs = durationMs;
  if (lastMapRenderDurationMs > maxMapRenderDurationMs) maxMapRenderDurationMs = lastMapRenderDurationMs;
  lastMapRenderFinishedMs = millis();
  mapRenderCount++;
  if (drewTiles) mapRenderDraws++;
  if (cacheHit) mapRenderCacheHits++;
}

static void recordMapRenderTiming(uint32_t startedMs, bool drewTiles, bool cacheHit) {
  recordMapRenderTimingDuration(millis() - startedMs, drewTiles, cacheHit);
}

static bool max17048ReadReg16(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MAX17048_ADDR, (uint8_t)2) != 2) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  value = ((uint16_t)msb << 8) | lsb;
  return true;
}

static bool max17048WriteReg16(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  return Wire.endTransmission() == 0;
}

static const char* i2cKnownDeviceName(uint8_t addr) {
  switch (addr) {
    case TOUCH_ADDR: return "touch";
    case CYD_AUDIO_CODEC_ADDR: return "codec";
    case MAX17048_ADDR: return "max17048";
    default: return nullptr;
  }
}

static size_t scanI2cPair(uint8_t sda, uint8_t scl, char* out, size_t outSize) {
  if (!out || outSize == 0) return 0;
  out[0] = '\0';

  Wire.end();
  delay(2);
  Wire.begin(sda, scl, 100000);
  Wire.setClock(100000);
  delay(2);

  uint8_t found[32];
  size_t foundCount = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0 && foundCount < sizeof(found)) {
      found[foundCount++] = addr;
    }
  }

  Wire.end();
  delay(2);
  Wire.begin(TOUCH_SDA, TOUCH_SCL, 100000);
  Wire.setClock(100000);
  delay(2);

  size_t used = 0;
  used += snprintf(out + used, outSize - used, "SDA %u / SCL %u: ", sda, scl);
  if (foundCount == 0) {
    used += snprintf(out + used, outSize > used ? outSize - used : 0, "no devices found");
    return foundCount;
  }

  for (size_t i = 0; i < foundCount && used < outSize - 1; i++) {
    const char* name = i2cKnownDeviceName(found[i]);
    if (i) used += snprintf(out + used, outSize - used, ", ");
    if (name) used += snprintf(out + used, outSize - used, "0x%02X(%s)", found[i], name);
    else used += snprintf(out + used, outSize - used, "0x%02X", found[i]);
  }
  return foundCount;
}

static String buildI2cProbeJson() {
  char docs[256];
  char swapped[256];
  size_t docsFound = scanI2cPair(TOUCH_SDA, TOUCH_SCL, docs, sizeof(docs));
  size_t swappedFound = scanI2cPair(TOUCH_SCL, TOUCH_SDA, swapped, sizeof(swapped));

  snprintf(i2cProbeSummary, sizeof(i2cProbeSummary),
           "Docs map %s, swapped map %s.",
           docsFound ? "answers" : "does not answer",
           swappedFound ? "answers" : "does not answer");
  snprintf(i2cProbeDetails, sizeof(i2cProbeDetails),
           "%s\n%s\nExpect touch=0x38, codec=0x18, fuel gauge=0x36 on the correct map.",
           docs, swapped);

  String json;
  json.reserve(700);
  json = "{";
  json += "\"summary\":\"" + jsonEscape(i2cProbeSummary) + "\",";
  json += "\"details\":\"" + jsonEscape(i2cProbeDetails) + "\",";
  json += "\"runs\":[";
  json += "{\"label\":\"docs\",\"" "sda\":" + String(TOUCH_SDA) + ",\"scl\":" + String(TOUCH_SCL) + ",\"found\":" + String(docsFound) + ",\"text\":\"" + jsonEscape(docs) + "\"},";
  json += "{\"label\":\"swapped\",\"sda\":" + String(TOUCH_SCL) + ",\"scl\":" + String(TOUCH_SDA) + ",\"found\":" + String(swappedFound) + ",\"text\":\"" + jsonEscape(swapped) + "\"}";
  json += "]}";
  return json;
}

static bool readMax17048ChargeRate(float& percentPerHour) {
  uint16_t rawCrate = 0;
  if (!max17048ReadReg16(0x16, rawCrate)) return false;
  percentPerHour = (float)((int16_t)rawCrate) * 0.208f;
  return true;
}

static constexpr uint8_t BATTERY_VOLTAGE_QUALIFIED_SAMPLES = 8;
static constexpr uint32_t BATTERY_VOLTAGE_ALERT_MS = 30000;
static constexpr uint32_t BATTERY_LOW_VOLTAGE_MV = 3650;
static constexpr uint32_t BATTERY_CRITICAL_VOLTAGE_MV = 3500;
static constexpr int BATTERY_LOW_SOC_PERCENT = 20;
static constexpr int BATTERY_CRITICAL_SOC_PERCENT = 10;

static int estimateLiPoPercentFromMv(uint32_t mv) {
  if (mv >= 4200) return 100;
  if (mv >= 4100) return map(mv, 4100, 4200, 90, 100);
  if (mv >= 4000) return map(mv, 4000, 4100, 75, 90);
  if (mv >= 3900) return map(mv, 3900, 4000, 55, 75);
  if (mv >= 3800) return map(mv, 3800, 3900, 35, 55);
  if (mv >= 3700) return map(mv, 3700, 3800, 20, 35);
  if (mv >= 3600) return map(mv, 3600, 3700, 10, 20);
  if (mv >= 3500) return map(mv, 3500, 3600, 5, 10);
  if (mv >= 3400) return map(mv, 3400, 3500, 0, 5);
  return 0;
}

static uint32_t filterBatteryVoltage(uint32_t rawMv, bool chargeRateValid, float chargeRate) {
  localBattery.rawMv = rawMv;
  localBattery.rawPackMv = rawMv;
  localBattery.sampleCount++;

  if (localBattery.filteredPackMv == 0 || localBattery.sampleCount <= 1) {
    localBattery.filteredPackMv = rawMv;
    localBattery.batteryMv = rawMv;
    localBattery.learnedBatteryMv = rawMv;
    localBattery.voltageStableSamples = 1;
    localBattery.voltageUnstable = false;
    return rawMv;
  }

  int32_t diffMv = (int32_t)rawMv - (int32_t)localBattery.filteredPackMv;
  uint32_t absDiffMv = (uint32_t)abs(diffMv);
  bool activeChargeChange = chargeRateValid && fabsf(chargeRate) > 8.0f;
  bool outlier = absDiffMv > (activeChargeChange ? 140UL : 80UL);
  uint32_t maxStepMv = activeChargeChange ? 25UL : 10UL;

  if (outlier) {
    localBattery.voltageRejectedSamples++;
    localBattery.voltageStableSamples = 0;
    localBattery.voltageUnstable = true;
    maxStepMv = localBattery.sampleCount < BATTERY_VOLTAGE_QUALIFIED_SAMPLES ? 25 : 3;
  } else {
    if (localBattery.voltageStableSamples < 255) localBattery.voltageStableSamples++;
    if (localBattery.voltageStableSamples >= BATTERY_VOLTAGE_QUALIFIED_SAMPLES) localBattery.voltageUnstable = false;
  }

  if (absDiffMv <= maxStepMv) {
    localBattery.filteredPackMv = rawMv;
  } else if (diffMv > 0) {
    localBattery.filteredPackMv += maxStepMv;
  } else if (localBattery.filteredPackMv > maxStepMv) {
    localBattery.filteredPackMv -= maxStepMv;
  } else {
    localBattery.filteredPackMv = rawMv;
  }

  localBattery.batteryMv = localBattery.filteredPackMv;
  localBattery.learnedBatteryMv = localBattery.filteredPackMv;
  return localBattery.filteredPackMv;
}

static bool batteryVoltageQualified() {
  return !localBattery.voltageUnstable &&
         localBattery.sampleCount >= BATTERY_VOLTAGE_QUALIFIED_SAMPLES &&
         localBattery.voltageStableSamples >= BATTERY_VOLTAGE_QUALIFIED_SAMPLES;
}

static const char* batteryAlertLabel() {
  if (localBattery.batteryCritical) return "critical";
  if (localBattery.batteryLow) return "low";
  return "ok";
}

static void updateBatteryAlertState(int displayedPercent, uint32_t packMv, bool charging, uint32_t nowMs) {
  bool voltageCanAlert = batteryVoltageQualified() && !charging;

  if (voltageCanAlert && packMv <= BATTERY_LOW_VOLTAGE_MV) {
    if (!localBattery.lowVoltageSinceMs) localBattery.lowVoltageSinceMs = nowMs;
  } else {
    localBattery.lowVoltageSinceMs = 0;
  }

  if (voltageCanAlert && packMv <= BATTERY_CRITICAL_VOLTAGE_MV) {
    if (!localBattery.criticalVoltageSinceMs) localBattery.criticalVoltageSinceMs = nowMs;
  } else {
    localBattery.criticalVoltageSinceMs = 0;
  }

  localBattery.voltageLowSustained =
    localBattery.lowVoltageSinceMs && nowMs - localBattery.lowVoltageSinceMs >= BATTERY_VOLTAGE_ALERT_MS;
  localBattery.voltageCriticalSustained =
    localBattery.criticalVoltageSinceMs && nowMs - localBattery.criticalVoltageSinceMs >= BATTERY_VOLTAGE_ALERT_MS;
  localBattery.batteryCritical =
    displayedPercent <= BATTERY_CRITICAL_SOC_PERCENT || localBattery.voltageCriticalSustained;
  localBattery.batteryLow =
    localBattery.batteryCritical ||
    displayedPercent <= BATTERY_LOW_SOC_PERCENT ||
    localBattery.voltageLowSustained;
}

static int smoothDisplayedBatteryPercent(int targetPercent, bool charging) {
  targetPercent = constrain(targetPercent, 0, 100);
  if (!localBattery.lastGaugeMs) return targetPercent;
  int diff = targetPercent - localBattery.percent;
  if (abs(diff) <= 1) return targetPercent;
  int maxStep = charging ? 2 : 1;
  if (diff > 0) return localBattery.percent + min(diff, maxStep);
  return localBattery.percent - min(-diff, maxStep);
}

static void updateBatteryTrend(uint32_t packMv, uint32_t nowMs) {
  static constexpr uint8_t TREND_WINDOW = sizeof(localBattery.trendMv) / sizeof(localBattery.trendMv[0]);
  localBattery.trendMv[localBattery.trendHead] = packMv;
  localBattery.trendMs[localBattery.trendHead] = nowMs;
  localBattery.trendHead = (localBattery.trendHead + 1) % TREND_WINDOW;
  if (localBattery.trendCount < TREND_WINDOW) localBattery.trendCount++;
  localBattery.trendSampleCount = localBattery.trendCount;

  if (localBattery.trendCount < 4) {
    localBattery.deltaMvPerMin = 0;
    localBattery.deltaMvPerMinTenths = 0;
    return;
  }

  uint8_t newestIndex = (localBattery.trendHead + TREND_WINDOW - 1) % TREND_WINDOW;
  uint8_t oldestIndex = (localBattery.trendHead + TREND_WINDOW - localBattery.trendCount) % TREND_WINDOW;
  uint32_t newestMs = localBattery.trendMs[newestIndex];
  uint32_t oldestMs = localBattery.trendMs[oldestIndex];
  if (newestMs <= oldestMs) return;

  uint32_t windowMs = newestMs - oldestMs;
  int32_t deltaMv = (int32_t)localBattery.trendMv[newestIndex] - (int32_t)localBattery.trendMv[oldestIndex];
  if (windowMs < 20000 || abs(deltaMv) < 3) {
    localBattery.deltaMvPerMin = 0;
    localBattery.deltaMvPerMinTenths = 0;
    return;
  }

  localBattery.deltaMvPerMinTenths = (int32_t)((int64_t)deltaMv * 600000LL / (int64_t)windowMs);
  localBattery.deltaMvPerMin = localBattery.deltaMvPerMinTenths / 10;
}

static bool decideBatteryCharging(bool chargeRateValid, float chargeRate, uint32_t packMv, uint32_t nowMs) {
  int32_t deltaMv = 0;
  uint32_t elapsedMs = 0;
  if (localBattery.previousSampleMs) {
    deltaMv = (int32_t)packMv - (int32_t)localBattery.previousPackMv;
    elapsedMs = nowMs - localBattery.previousSampleMs;
  }
  localBattery.instantDeltaMv = deltaMv;
  localBattery.previousPackMv = packMv;
  localBattery.previousSampleMs = nowMs;

  if (!chargeRateValid) return localBattery.charging;
  if (chargeRate >= 1.0f) return true;
  if (chargeRate <= 0.0f) return false;

  bool fastVoltageDrop = elapsedMs > 0 && elapsedMs <= 4000 && deltaMv <= -5;
  if (localBattery.charging && fastVoltageDrop) return false;
  if (!localBattery.charging && chargeRate >= 0.4f) return true;
  return localBattery.charging;
}

static int chooseDisplayedBatteryPercent(float soc, int voltagePercent, bool socReliable, bool charging) {
  localBattery.gaugePercent = constrain((int)roundf(soc), 0, 100);
  float correctedSoc = soc + ((float)localBattery.calibrationOffsetTenths / 10.0f);
  if (correctedSoc < 0.0f) correctedSoc = 0.0f;
  if (correctedSoc > 100.0f) correctedSoc = 100.0f;
  localBattery.correctedGaugeSoc = correctedSoc;
  localBattery.correctedGaugePercent = constrain((int)roundf(correctedSoc), 0, 100);
  localBattery.calibrationOffsetMv = 0;

  if (!socReliable) {
    strlcpy(localBattery.percentSource,
            batteryVoltageQualified() ? "Voltage estimate" : "Voltage estimate (settling)",
            sizeof(localBattery.percentSource));
    return voltagePercent;
  }

  strlcpy(localBattery.percentSource,
          localBattery.voltageUnstable ? "MAX17048 SOC (voltage noisy)" :
          (localBattery.calibrationOffsetTenths ? "MAX17048 SOC + trim" : "MAX17048 SOC"),
          sizeof(localBattery.percentSource));
  return localBattery.correctedGaugePercent;
}

static void updateBatteryLearning(float soc, int voltagePercent, bool socReliable, bool charging,
                                  bool chargeRateValid, float chargeRate, uint32_t nowMs) {
  if (!socReliable || charging) {
    localBattery.stableSampleCount = 0;
    return;
  }

  if (chargeRateValid && fabsf(chargeRate) > 1.5f) {
    localBattery.stableSampleCount = 0;
    return;
  }

  if (abs(localBattery.instantDeltaMv) > 3) {
    localBattery.stableSampleCount = 0;
    return;
  }

  int targetOffsetTenths = (int)roundf(((float)voltagePercent - soc) * 10.0f);
  targetOffsetTenths = constrain(targetOffsetTenths, -120, 120);
  int errorTenths = targetOffsetTenths - localBattery.calibrationOffsetTenths;
  if (abs(errorTenths) < 10) {
    localBattery.stableSampleCount = min<uint8_t>(localBattery.stableSampleCount + 1, 255);
    return;
  }

  localBattery.stableSampleCount = min<uint8_t>(localBattery.stableSampleCount + 1, 255);
  if (localBattery.stableSampleCount < 30) return;

  localBattery.calibrationOffsetTenths += errorTenths > 0 ? 1 : -1;
  localBattery.calibrationOffsetTenths = constrain(localBattery.calibrationOffsetTenths, -120, 120);
  localBattery.stableSampleCount = 0;

  if (!localBattery.lastLearnSaveMs || nowMs - localBattery.lastLearnSaveMs > 60000) {
    prefs.putShort("batTrim10", localBattery.calibrationOffsetTenths);
    localBattery.lastLearnSaveMs = nowMs;
  }
}

static bool sampleMax17048() {
  uint16_t rawVcell = 0;
  uint16_t rawSoc = 0;
  if (!max17048ReadReg16(0x02, rawVcell) || !max17048ReadReg16(0x04, rawSoc)) {
    localBattery.gaugePresent = false;
    localBattery.batteryLow = false;
    localBattery.batteryCritical = false;
    localBattery.voltageLowSustained = false;
    localBattery.voltageCriticalSustained = false;
    strlcpy(localBattery.powerState, "gauge missing", sizeof(localBattery.powerState));
    return false;
  }

  uint16_t rawVersion = 0;
  if (max17048ReadReg16(0x08, rawVersion)) localBattery.gaugeVersion = rawVersion;

  uint32_t rawPackMv = (uint32_t)(((rawVcell >> 4) * 125UL + 50UL) / 100UL);
  float soc = (float)(rawSoc >> 8) + ((float)(rawSoc & 0xFF) / 256.0f);
  if (soc < 0.0f) soc = 0.0f;
  if (soc > 100.0f) soc = 100.0f;
  float chargeRate = 0.0f;
  bool chargeRateValid = readMax17048ChargeRate(chargeRate);
  uint32_t packMv = filterBatteryVoltage(rawPackMv, chargeRateValid, chargeRate);
  int voltagePercent = estimateLiPoPercentFromMv(packMv);
  bool socReliable = !(soc < 1.0f && packMv > 3350);
  if (!socReliable && !localBattery.quickStartSent) {
    if (max17048WriteReg16(0x06, 0x4000)) {
      localBattery.quickStartSent = true;
      appendLine(eventLog, LOG_SIZE, "[battery] MAX17048 quick-start sent\n");
    }
  }

  uint32_t now = millis();
  bool charging = decideBatteryCharging(chargeRateValid, chargeRate, packMv, now);
  updateBatteryLearning(soc, voltagePercent, socReliable, charging, chargeRateValid, chargeRate, now);
  int displayedPercent = smoothDisplayedBatteryPercent(chooseDisplayedBatteryPercent(soc, voltagePercent, socReliable, charging), charging);
  updateBatteryAlertState(displayedPercent, packMv, charging, now);
  localBattery.gaugePresent = true;
  localBattery.lastGaugeMs = now;
  localBattery.rawGaugeSoc = rawSoc;
  localBattery.gaugeSoc = soc;
  localBattery.gaugeSocReliable = socReliable;
  localBattery.voltagePercent = voltagePercent;
  localBattery.chargeRateValid = chargeRateValid;
  localBattery.chargeRatePercentHr = chargeRateValid ? chargeRate : 0.0f;
  localBattery.charging = charging;
  localBattery.percent = displayedPercent;
  updateBatteryTrend(packMv, now);
  strlcpy(localBattery.powerState, localBattery.percentSource, sizeof(localBattery.powerState));
  return true;
}

static void sampleLocalBattery() {
  if (localBattery.lastGaugeMs && millis() - localBattery.lastGaugeMs < 1000) return;
  sampleMax17048();
}

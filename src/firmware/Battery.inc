static_assert(BATTERY_DIVIDER_TOP_OHMS > 0 && BATTERY_DIVIDER_BOTTOM_OHMS > 0, "Battery divider resistances must be positive");
static_assert(BATTERY_ADC_SCALE > 0, "Battery ADC calibration must be positive");
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

static const char* i2cKnownDeviceName(uint8_t addr) {
  switch (addr) {
    case TOUCH_ADDR: return "touch";
    case BATTERY_ADS1115_ADDR: return "ads1115";
    case CYD_AUDIO_CODEC_ADDR: return "codec";
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
           "%s\n%s\nExpect touch=0x38, codec=0x18, battery ADC=0x48 on the correct map.",
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

// No charger-status hardware is installed.
static const char* batteryChargingStatus = "unknown";
static constexpr bool batteryChargeStatusValid = false;
static constexpr bool batteryActivelyCharging = false;

static const char* batteryAlertLabel() {
  if (!localBattery.voltageValid) return "unavailable";
  if (localBattery.batteryCritical) return "critical";
  if (localBattery.batteryLow) return "low";
  return "ok";
}

static void updateBatteryVoltage(uint32_t mv, bool valid, uint32_t now) {
  // Reject implausible readings before updating the estimate.
  localBattery.voltageValid = valid;
  localBattery.gaugeSocReliable = false;
  localBattery.chargeRateValid = false;
  localBattery.charging = batteryActivelyCharging;
  batteryRuntime.pause();
  if (!valid) {
    ++localBattery.readErrors;
    localBattery.batteryMv = localBattery.rawPackMv = localBattery.filteredPackMv = 0;
    localBattery.percent = localBattery.gaugePercent = -1;
    localBattery.gaugeSoc = -1.0f;
    localBattery.rawMv = 0;
    localBattery.sampleCount = 0;
    localBattery.lowVoltageSinceMs = localBattery.criticalVoltageSinceMs = 0;
    localBattery.batteryLow = localBattery.batteryCritical = false;
    strlcpy(localBattery.powerState, "Battery: Unavailable", sizeof(localBattery.powerState));
    return;
  }
  localBattery.rawPackMv = localBattery.rawMv = mv;
  localBattery.filteredPackMv = localBattery.sampleCount ? (localBattery.filteredPackMv * 7 + mv) / 8 : mv;
  ++localBattery.sampleCount;
  localBattery.batteryMv = localBattery.filteredPackMv;
  localBattery.lastGaugeMs = now;
  localBattery.percent = BatteryVoltage::estimatedPercent(localBattery.batteryMv);
  localBattery.gaugePercent = localBattery.percent;
  localBattery.gaugeSoc = localBattery.percent;
  // Require sustained low voltage; percentage is only a rough voltage estimate.
  if (localBattery.batteryMv <= 3650) {
    if (!localBattery.lowVoltageSinceMs) localBattery.lowVoltageSinceMs = now;
  } else localBattery.lowVoltageSinceMs = 0;
  if (localBattery.batteryMv <= 3500) {
    if (!localBattery.criticalVoltageSinceMs) localBattery.criticalVoltageSinceMs = now;
  } else localBattery.criticalVoltageSinceMs = 0;
  localBattery.batteryLow = localBattery.lowVoltageSinceMs && now - localBattery.lowVoltageSinceMs >= 30000;
  localBattery.batteryCritical = localBattery.criticalVoltageSinceMs && now - localBattery.criticalVoltageSinceMs >= 30000;
  strlcpy(localBattery.percentSource, "ADS1115 voltage estimate", sizeof(localBattery.percentSource));
  strlcpy(localBattery.powerState, "Battery: Estimated level", sizeof(localBattery.powerState));
}

static bool readBatteryAdcRegister(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(BATTERY_ADS1115_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)BATTERY_ADS1115_ADDR, (uint8_t)2) != 2) return false;
  value = (uint16_t)Wire.read() << 8;
  value |= (uint8_t)Wire.read();
  return true;
}

// Let the loop service touch/radio while the ADC converts; never delay for conversion.
static void sampleLocalBattery() {
  static bool pending = false;
  static uint32_t started = 0, lastPoll = 0;
  uint32_t now = millis();
  if (!pending) {
    if (localBattery.lastSampleMs && uint32_t(now - localBattery.lastSampleMs) < 1000) return;
    localBattery.lastSampleMs = now;
    ++localBattery.readAttempts;
    Wire.beginTransmission(BATTERY_ADS1115_ADDR);
    Wire.write((uint8_t)1);
    Wire.write((uint8_t)(Ads1115::CONFIG >> 8));
    Wire.write((uint8_t)Ads1115::CONFIG);
    if (Wire.endTransmission() != 0) { updateBatteryVoltage(0, false, now); return; }
    pending = true; started = lastPoll = now;
    return;
  }
  if (uint32_t(now - lastPoll) < 10) return;
  lastPoll = now;
  uint16_t config = 0, raw = 0;
  bool ok = readBatteryAdcRegister(1, config) && Ads1115::configured(config);
  if (ok && !Ads1115::ready(config) && uint32_t(now - started) < 100) return;
  ok = ok && Ads1115::ready(config) && readBatteryAdcRegister(0, raw) && Ads1115::usable(raw);
  pending = false;
  uint32_t mv = ok ? Ads1115::cellMillivolts(raw, BATTERY_DIVIDER_TOP_OHMS,
                  BATTERY_DIVIDER_BOTTOM_OHMS, BATTERY_ADC_SCALE) : 0;
  updateBatteryVoltage(mv, ok && BatteryVoltage::valid(mv), now);
}

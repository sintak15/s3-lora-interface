// Internal implementation module; included once by the main sketch.
static ToneStyle clampToneStyle(uint8_t style) {
  return style < TONE_STYLE_COUNT ? (ToneStyle)style : TONE_STYLE_SOFT_BOOP;
}

static const char* toneStyleName(ToneStyle style) {
  return NotificationSound::preset(style).name;
}

static void setToneDropdownOptions(lv_obj_t* dropdown) {
  lv_dropdown_clear_options(dropdown);
  for (uint8_t style = 0; style < TONE_STYLE_COUNT; ++style)
    lv_dropdown_add_option(dropdown, toneStyleName((ToneStyle)style), LV_DROPDOWN_POS_LAST);
}

static ToneStyle toneStyleForKind(ToneKind kind) {
  switch (kind) {
    case TONE_DIRECT: return toneDirectStyle;
    case TONE_TEST: return toneMessageStyle;
    case TONE_MESSAGE: return toneMessageStyle;
    default: return TONE_STYLE_SOFT_BOOP;
  }
}

static const ToneSegment* tonePatternFor(ToneStyle style, size_t& count) {
  const auto& preset = NotificationSound::preset(style);
  count = preset.count;
  return preset.segments;
}


// Unity codec gain plus bounded PCM amplitude gives every volume setting headroom.
static uint8_t toneCodecVolumeRegister() { return toneVolumePercent ? 0xBF : 0x00; }
static float toneCodecGainDb() { return toneVolumePercent ? 0.0f : -95.5f; }

static void toneSetSpeakerEnabled(bool enabled) {
  toneSpeakerEnabled = enabled;
  if (CYD_AUDIO_ENABLE_PIN >= 0) {
    const int off = CYD_AUDIO_ENABLE_LEVEL == LOW ? HIGH : LOW;
    digitalWrite(CYD_AUDIO_ENABLE_PIN, enabled ? CYD_AUDIO_ENABLE_LEVEL : off);
  }
  toneWorker.ampEnabled.store(enabled);
}

static void requestToneRecovery(bool manual) {
  if (manual) { toneRecoveryAttempts = 0; toneRecoveryAttempted = false; }
  toneWorker.epoch.fetch_add(1);
  toneWorker.pauseRequested.store(true);
  toneI2sReady = toneCodecReady = false;
  toneSetSpeakerEnabled(false);
}

static bool toneCodecWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission((uint8_t)CYD_AUDIO_CODEC_ADDR);
  Wire.write(reg); Wire.write(value);
  bool ok = Wire.endTransmission() == 0;
  if (!ok) ++toneCodecIoFailures;
  return ok;
}

static bool toneCodecReadChecked(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission((uint8_t)CYD_AUDIO_CODEC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom((uint8_t)CYD_AUDIO_CODEC_ADDR, (uint8_t)1) != 1) {
    ++toneCodecIoFailures;
    return false;
  }
  value = Wire.read();
  return true;
}

static int toneCodecReadInt(uint8_t reg) {
  if (!toneCodecReady) return -1;
  uint8_t value;
  if (toneCodecReadChecked(reg, value)) return value;
  requestToneRecovery(false);
  return -1; // Never disguise an I2C failure as the valid register value 0xFF.
}

static bool initToneCodec() {
  uint8_t id1 = 0, id2 = 0;
  bool ok = toneCodecReadChecked(0xFD, id1) && toneCodecReadChecked(0xFE, id2);
  if (!ok || id1 != 0x83 || id2 != 0x11) { ++toneCodecInitFailures; return false; }
  // ES8311 slave, external MCLK=256*16 kHz, 16-bit Philips I2S. Register 04
  // matches Espressif's 4.096 MHz/16 kHz DAC oversampling coefficient (0x20).
  // ADC data is muted AND tri-stated; OUTP/OUTN use line drive into the board PA.
  static const uint8_t config[][2] = {
    {0x44,0x08}, {0x44,0x08}, {0x01,0x30}, {0x02,0x00},
    {0x03,0x10}, {0x04,0x20}, {0x05,0x00}, {0x0B,0x00},
    {0x0C,0x00}, {0x10,0x1F}, {0x11,0x7F}, {0x00,0x80},
    {0x01,0x35}, {0x06,0x07}, {0x07,0x10}, {0x08,0xFF},
    {0x09,0x0C}, {0x0A,0x4C}, {0x0D,0x01}, {0x0E,0x62},
    {0x12,0x00}, {0x13,0x00}, {0x37,0x48}, {0x45,0x00}
  };
  for (const auto& setting : config) {
    if (!toneCodecWrite(setting[0], setting[1])) { ok = false; break; }
    if (setting[0] == 0x00) delay(12);
  }
  ok = ok && toneCodecWrite(0x32, toneCodecVolumeRegister());
  ok = ok && toneCodecWrite(0x31, toneVolumePercent ? 0x00 : 0x60);
  uint8_t value = 0;
  ok = ok && toneCodecReadChecked(0x04, value) && value == 0x20;
  ok = ok && toneCodecReadChecked(0x09, value) && value == 0x0C;
  ok = ok && toneCodecReadChecked(0x32, value) && value == toneCodecVolumeRegister();
  if (!ok) ++toneCodecInitFailures;
  return ok;
}

static bool beginToneI2s() {
  // GPIO6 is the codec's recording output. Never configure it as a transmitter.
  pinMode(6, INPUT);
  toneI2s.setPins(CYD_AUDIO_BCLK_PIN, CYD_AUDIO_LRCLK_PIN, CYD_AUDIO_DOUT_PIN, -1, CYD_AUDIO_MCLK_PIN);
  return toneI2s.begin(I2S_MODE_STD, TONE_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
}

static void applyToneVolume() {
  toneVolumePercent = constrain((int)toneVolumePercent, 0, 100);
  toneWorker.volume.store(toneVolumePercent);
  if (!toneVolumePercent) {
    toneWorker.epoch.fetch_add(1); // Cancel queued/current sounds without resetting I2S.
    toneSetSpeakerEnabled(false);
  }
  if (toneCodecReady && (!toneCodecWrite(0x32, toneCodecVolumeRegister()) ||
                         !toneCodecWrite(0x31, toneVolumePercent ? 0x00 : 0x60))) {
    requestToneRecovery(false);
  }
}

static const char* toneAudioStateLabel() {
  if (!toneTaskHandle) return "Audio unavailable";
  if (toneWorker.pauseRequested.load()) return toneRecoveryAttempts >= 3 ? "Audio fault" : "Recovering audio";
  return toneI2sReady && toneCodecReady ? (toneActive ? "Playing" : "Ready") : "Audio unavailable";
}

static void updateToneVolumeLabel() {
  if (!lblToneVolume) return;
  char text[176];
  snprintf(text, sizeof(text), "Volume: %u%%  %s\nMsg: %s  DM: %s",
           toneVolumePercent, toneAudioStateLabel(), toneStyleName(toneMessageStyle), toneStyleName(toneDirectStyle));
  setReadoutText(lblToneVolume, text);
}

static void toneVolumeSliderEvent(lv_event_t* e) {
  toneVolumePercent = constrain((int)lv_slider_get_value((lv_obj_t*)lv_event_get_target(e)), 0, 100);
  prefs.putUChar("toneVol", toneVolumePercent);
  applyToneVolume(); updateToneVolumeLabel();
}

static void updateToneDropdowns() {
  if (dropdownToneMessage && lv_dropdown_get_selected(dropdownToneMessage) != (uint16_t)toneMessageStyle)
    lv_dropdown_set_selected(dropdownToneMessage, (uint16_t)toneMessageStyle);
  if (dropdownToneDirect && lv_dropdown_get_selected(dropdownToneDirect) != (uint16_t)toneDirectStyle)
    lv_dropdown_set_selected(dropdownToneDirect, (uint16_t)toneDirectStyle);
}

static void toneMessageDropdownEvent(lv_event_t* e) {
  toneMessageStyle = clampToneStyle((uint8_t)lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e)));
  prefs.putUChar("toneMsg", (uint8_t)toneMessageStyle); updateToneVolumeLabel();
}
static void toneDirectDropdownEvent(lv_event_t* e) {
  toneDirectStyle = clampToneStyle((uint8_t)lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e)));
  prefs.putUChar("toneDir", (uint8_t)toneDirectStyle); updateToneVolumeLabel();
}

static bool enqueueTone(ToneKind kind, ToneStyle style, bool test, uint8_t volumeLimit = 100) {
  if (!toneTaskHandle || !toneRequestQueue || !toneVolumePercent || kind == TONE_NONE) return false;
  if (!toneI2sReady || !toneCodecReady || toneWorker.pauseRequested.load()) {
    if (toneRecoveryAttempts >= 3) requestToneRecovery(true);
    return false;
  }
  unsigned jobs = toneWorker.jobs.load();
  if ((test && jobs) || jobs >= 2) { toneWorker.dropped.fetch_add(1); return false; }
  ToneRequest request{kind, style, toneWorker.epoch.load(), volumeLimit};
  toneWorker.jobs.fetch_add(1);
  toneWorker.nextKind.store(kind);
  BaseType_t sent = kind == TONE_DIRECT ? xQueueSendToFront(toneRequestQueue, &request, 0) :
                                         xQueueSend(toneRequestQueue, &request, 0);
  if (sent == pdTRUE) return true;
  toneWorker.jobs.fetch_sub(1); toneWorker.dropped.fetch_add(1);
  return false;
}

static void queueTone(ToneKind kind) {
  enqueueTone(kind, toneStyleForKind(kind), kind == TONE_TEST);
}
static bool queueToneStyle(ToneStyle style) {
  return enqueueTone(TONE_TEST, clampToneStyle((uint8_t)style), true);
}
static int8_t toneStyleIndexFromText(String value) {
  value.trim();
  value.toLowerCase();
  if (!value.length()) return -1;
  for (uint8_t style = 0; style < TONE_STYLE_COUNT; ++style)
    if (value.equalsIgnoreCase(toneStyleName((ToneStyle)style))) return (int8_t)style;
  if (isDigit(value[0])) {
    int index = value.toInt();
    return (index >= 0 && index < TONE_STYLE_COUNT) ? (int8_t)index : -1;
  }
  if (value == "low bell" || value == "bwomp") return TONE_STYLE_SOFT_BOOP;
  if (value == "chime") return TONE_STYLE_DOUBLE_BOOP;
  if (value == "two step" || value == "double") return TONE_STYLE_TWO_NOTE;
  if (value == "beacon" || value == "pulse") return TONE_STYLE_TRIPLE_BOOP;
  if (value == "pager" || value == "alert") return TONE_STYLE_GENTLE_ALERT;
  if (value == "bell" || value == "loud") return TONE_STYLE_RISING_BOOP;
  if (value == "sustain" || value == "carrier" || value == "test") return TONE_STYLE_LONG_BOOP;
  if (value == "test sine" || value == "sine" || value == "raw" || value == "square" || value == "tone") return TONE_STYLE_TEST_SINE;
  return -1;
}

static String buildToneStatusJson() {
  String json = "{";
  json += "\"toneVolume\":" + String(toneVolumePercent) + ",";
  // Fixed values preserve the retired boost/profile status fields for clients.
  json += "\"toneBoost\":\"0 dB\",";
  json += "\"toneDrive\":" + String(TONE_PCM_DRIVE_PERCENT) + ",";
  json += "\"toneCodecProfile\":\"Speaker\",";
  json += "\"toneCodecProfileIndex\":0,";
  json += "\"toneMessage\":\"" + String(toneStyleName(toneMessageStyle)) + "\",";
  json += "\"toneDirect\":\"" + String(toneStyleName(toneDirectStyle)) + "\",";
  json += "\"toneCodecReg\":" + String(toneCodecVolumeRegister()) + ",";
  json += "\"toneCodecGainDb\":" + String(toneCodecGainDb(), 1) + ",";
  json += "\"toneAudioReady\":" + String(toneI2sReady && toneTaskHandle && !toneWorker.pauseRequested.load() ? "true" : "false") + ",";
  json += "\"toneCodecReady\":" + String(toneCodecReady && !toneWorker.pauseRequested.load() ? "true" : "false") + ",";
  json += "\"toneActive\":" + String(toneActive ? "true" : "false") + ",";
  json += "\"tonePending\":" + String((int)pendingToneKind) + ",";
  json += "\"toneLastMs\":" + String(lastToneMs) + ",";
  json += "\"toneSpeakerEnabled\":" + String(toneSpeakerEnabled ? "true" : "false") + ",";
  json += "\"toneAmpPin\":" + String(CYD_AUDIO_ENABLE_PIN >= 0 ? digitalRead(CYD_AUDIO_ENABLE_PIN) : -1) + ",";
  json += "\"toneAmpEnableLevel\":" + String((int)CYD_AUDIO_ENABLE_LEVEL) + ",";
  json += "\"toneCodecInitFailures\":" + String(toneCodecInitFailures) + ",";
  json += "\"toneCodecReg00\":" + String(toneCodecReadInt(0x00)) + ",";
  json += "\"toneCodecReg01\":" + String(toneCodecReadInt(0x01)) + ",";
  json += "\"toneCodecReg02\":" + String(toneCodecReadInt(0x02)) + ",";
  json += "\"toneCodecReg03\":" + String(toneCodecReadInt(0x03)) + ",";
  json += "\"toneCodecReg04\":" + String(toneCodecReadInt(0x04)) + ",";
  json += "\"toneCodecReg05\":" + String(toneCodecReadInt(0x05)) + ",";
  json += "\"toneCodecReg06\":" + String(toneCodecReadInt(0x06)) + ",";
  json += "\"toneCodecReg07\":" + String(toneCodecReadInt(0x07)) + ",";
  json += "\"toneCodecReg08\":" + String(toneCodecReadInt(0x08)) + ",";
  json += "\"toneCodecReg09\":" + String(toneCodecReadInt(0x09)) + ",";
  json += "\"toneCodecReg0A\":" + String(toneCodecReadInt(0x0A)) + ",";
  json += "\"toneCodecReg0B\":" + String(toneCodecReadInt(0x0B)) + ",";
  json += "\"toneCodecReg0C\":" + String(toneCodecReadInt(0x0C)) + ",";
  json += "\"toneCodecReg0D\":" + String(toneCodecReadInt(0x0D)) + ",";
  json += "\"toneCodecReg0E\":" + String(toneCodecReadInt(0x0E)) + ",";
  json += "\"toneCodecReg10\":" + String(toneCodecReadInt(0x10)) + ",";
  json += "\"toneCodecReg11\":" + String(toneCodecReadInt(0x11)) + ",";
  json += "\"toneCodecReg12\":" + String(toneCodecReadInt(0x12)) + ",";
  json += "\"toneCodecReg13\":" + String(toneCodecReadInt(0x13)) + ",";
  json += "\"toneCodecReg14\":" + String(toneCodecReadInt(0x14)) + ",";
  json += "\"toneCodecReg15\":" + String(toneCodecReadInt(0x15)) + ",";
  json += "\"toneCodecReg16\":" + String(toneCodecReadInt(0x16)) + ",";
  json += "\"toneCodecReg17\":" + String(toneCodecReadInt(0x17)) + ",";
  json += "\"toneCodecReg1B\":" + String(toneCodecReadInt(0x1B)) + ",";
  json += "\"toneCodecReg1C\":" + String(toneCodecReadInt(0x1C)) + ",";
  json += "\"toneCodecReg31\":" + String(toneCodecReadInt(0x31)) + ",";
  json += "\"toneCodecReg32\":" + String(toneCodecReadInt(0x32)) + ",";
  json += "\"toneCodecReg37\":" + String(toneCodecReadInt(0x37)) + ",";
  json += "\"toneCodecReg45\":" + String(toneCodecReadInt(0x45)) + ",";
  json += "\"toneCodecRegFD\":" + String(toneCodecReadInt(0xFD)) + ",";
  json += "\"toneCodecRegFE\":" + String(toneCodecReadInt(0xFE)) + ",";
  json += "\"toneWriteCalls\":" + String(toneWriteCalls) + ",";
  json += "\"toneBytesWritten\":" + String(toneBytesWritten) + ",";
  json += "\"toneLastWriteRequested\":" + String((unsigned)toneLastWriteRequested) + ",";
  json += "\"toneLastWriteBytes\":" + String((unsigned)toneLastWriteBytes) + ",";
  json += "\"toneShortWrites\":" + String(toneShortWrites) + ",";
  json += "\"toneWriteFailures\":" + String(toneWriteFailures) + ",";
  json += "\"toneDataPin\":" + String(CYD_AUDIO_DOUT_PIN) + ",";
  json += "\"toneWorkerReady\":" + String(toneTaskHandle ? "true" : "false") + ",";
  json += "\"toneQueued\":" + String(toneWorker.jobs.load()) + ",";
  json += "\"toneDropped\":" + String(toneWorker.dropped.load()) + ",";
  json += "\"toneRecovering\":" + String(toneWorker.pauseRequested.load() ? "true" : "false") + ",";
  json += "\"toneRecoveryAttempts\":" + String(toneRecoveryAttempts) + ",";
  json += "\"toneRecoveries\":" + String(toneRecoveries) + ",";
  json += "\"toneCodecIoFailures\":" + String(toneCodecIoFailures) + ",";
  json += "\"toneCodecRegFC\":" + String(toneCodecReadInt(0xFC)) + ",";
  json += "\"toneLastI2sError\":" + String(toneLastI2sError);
  json += "}";
  return json;
}


static bool toneRequestCancelled(uint32_t epoch) {
  return toneWorker.pauseRequested.load() || toneWorker.epoch.load() != epoch || !toneWorker.volume.load();
}

static bool writeToneBlock(const int16_t* frames, size_t bytes, uint32_t epoch, bool silence = false) {
  size_t offset = 0;
  uint32_t started = millis();
  while (offset < bytes) {
    if (toneWorker.pauseRequested.load() || (!silence && toneRequestCancelled(epoch))) return false;
    size_t requested = bytes - offset, sent = 0;
    // Use the native byte count: the Arduino wrapper loses bytes accepted on a
    // timeout. Retain and retry the unsent tail, never advance over missing data.
    esp_err_t error = i2s_channel_write(toneI2s.txChan(), (const uint8_t*)frames + offset, requested, &sent, 40);
    toneWorker.writeCalls.fetch_add(1);
    toneWorker.bytesWritten.fetch_add(sent);
    toneWorker.lastRequested.store(requested); toneWorker.lastWritten.store(sent);
    toneWorker.lastError.store(error);
    if (sent != requested) toneWorker.shortWrites.fetch_add(1);
    if (error != ESP_OK || !sent) toneWorker.failures.fetch_add(1);
    if (sent > requested || sent % (2 * sizeof(int16_t))) {
      toneWorker.lastError.store(ESP_ERR_INVALID_SIZE);
      toneWorker.pauseRequested.store(true); return false;
    }
    offset += sent;
    if ((error != ESP_OK && error != ESP_ERR_TIMEOUT) || (offset < bytes && millis() - started >= 160)) {
      toneWorker.pauseRequested.store(true); return false;
    }
    if (!sent) vTaskDelay(pdMS_TO_TICKS(1));
  }
  return true;
}

static bool drainToneAudio(uint32_t epoch) {
  int16_t silence[TONE_FRAMES_PER_SERVICE * 2] = {};
  for (uint32_t left = TONE_SAMPLE_RATE / 8; left;) {
    uint32_t length = min<uint32_t>(TONE_FRAMES_PER_SERVICE, left);
    if (!writeToneBlock(silence, length * 2 * sizeof(int16_t), epoch, true)) return false;
    left -= length;
  }
  return true;
}

static bool playToneRequest(const ToneRequest& request) {
  size_t count = 0;
  const ToneSegment* pattern = tonePatternFor(request.style, count);
  if (!pattern || !count) return false;
  int16_t frames[TONE_FRAMES_PER_SERVICE * 2];
  float phase = 0;
  for (size_t segmentIndex = 0; segmentIndex < count; ++segmentIndex) {
    const ToneSegment& segment = pattern[segmentIndex];
    const uint32_t total = max<uint32_t>(1, (uint32_t)segment.durationMs * TONE_SAMPLE_RATE / 1000);
    float phaseStep = 6.28318530718f * segment.frequency / TONE_SAMPLE_RATE;
    const float endFrequency = segment.endFrequency ? segment.endFrequency : segment.frequency;
    const float phaseStepDelta = total > 1 ?
        6.28318530718f * (endFrequency - segment.frequency) / (TONE_SAMPLE_RATE * (total - 1)) : 0;
    for (uint32_t position = 0; position < total;) {
      if (toneRequestCancelled(request.epoch)) return false;
      const uint32_t length = min<uint32_t>(TONE_FRAMES_PER_SERVICE, total - position);
      const float amplitude = TONE_PCM_PEAK * min<uint8_t>(toneWorker.volume.load(), request.volumeLimit) / 100.0f;
      for (uint32_t i = 0; i < length; ++i) {
        uint32_t index = position + i;
        float envelope = NotificationSound::envelope(index, total, TONE_SAMPLE_RATE, request.style == TONE_STYLE_TEST_SINE);
        int16_t sample = segment.frequency ? (int16_t)(sinf(phase) * amplitude * envelope) : 0;
        frames[i * 2] = frames[i * 2 + 1] = sample;
        phase += phaseStep;
        phaseStep += phaseStepDelta;
        if (phase >= 6.28318530718f) phase -= 6.28318530718f;
      }
      if (!writeToneBlock(frames, length * 2 * sizeof(int16_t), request.epoch)) return false;
      position += length;
    }
  }
  // The native write completes when DMA accepts data. Queue enough zeros to
  // drain the six 240-frame descriptors before declaring playback complete.
  return drainToneAudio(request.epoch);
}

static void toneAudioTask(void*) {
  uint32_t finishedMs = 0;
  for (;;) {
    if (toneWorker.pauseRequested.load()) {
      toneWorker.ampWanted.store(false);
      ToneRequest stale;
      while (xQueueReceive(toneRequestQueue, &stale, 0) == pdTRUE) {
        toneWorker.jobs.fetch_sub(1); toneWorker.dropped.fetch_add(1);
      }
      toneWorker.paused.store(true); // No I2S access until the main loop resumes us.
      while (toneWorker.pauseRequested.load()) vTaskDelay(pdMS_TO_TICKS(10));
      toneWorker.paused.store(false);
    }
    if (toneWorker.ampWanted.load() && millis() - finishedMs >= TONE_SPEAKER_HOLD_MS)
      toneWorker.ampWanted.store(false);
    ToneRequest request;
    if (xQueueReceive(toneRequestQueue, &request, pdMS_TO_TICKS(20)) != pdTRUE) continue;
    if (!toneRequestCancelled(request.epoch)) {
      toneWorker.active.store(true);
      toneWorker.startedMs.store(millis()); toneWorker.ampWanted.store(true);
      uint32_t waitStarted = millis();
      while (!toneWorker.ampEnabled.load() && !toneRequestCancelled(request.epoch) && millis() - waitStarted < 500)
        vTaskDelay(pdMS_TO_TICKS(2));
      if (toneWorker.ampEnabled.load() && !toneRequestCancelled(request.epoch)) {
        if (!playToneRequest(request)) {
          // Flush cancelled sounds while muted before accepting another test.
          if (!toneWorker.pauseRequested.load()) drainToneAudio(toneWorker.epoch.load());
          toneWorker.ampWanted.store(false);
        }
      } else {
        toneWorker.ampWanted.store(false); toneWorker.dropped.fetch_add(1);
      }
      finishedMs = millis();
      toneWorker.active.store(false);
    } else {
      toneWorker.dropped.fetch_add(1);
    }
    toneWorker.jobs.fetch_sub(1);
  }
}

static void serviceToneAudio() {
  toneActive = toneWorker.active.load();
  pendingToneKind = toneWorker.jobs.load() > (toneActive ? 1U : 0U) ? (ToneKind)toneWorker.nextKind.load() : TONE_NONE;
  lastToneMs = toneWorker.startedMs.load();
  toneWriteCalls = toneWorker.writeCalls.load(); toneBytesWritten = toneWorker.bytesWritten.load();
  toneShortWrites = toneWorker.shortWrites.load(); toneWriteFailures = toneWorker.failures.load();
  toneLastWriteRequested = toneWorker.lastRequested.load(); toneLastWriteBytes = toneWorker.lastWritten.load();
  toneLastI2sError = toneWorker.lastError.load();
  if (toneWorker.pauseRequested.load()) {
    toneSetSpeakerEnabled(false); toneI2sReady = toneCodecReady = false;
    // A failed backend never blocks the UI in a permanent retry loop. Three
    // attempts, at least two seconds apart; another user request can retry later.
    if (!toneTaskHandle || !toneWorker.paused.load() || toneRecoveryAttempts >= 3 ||
        (toneRecoveryAttempted && millis() - toneLastRecoveryMs < 2000)) return;
    toneRecoveryAttempted = true; toneLastRecoveryMs = millis(); ++toneRecoveryAttempts;
    toneI2s.end();
    toneI2sReady = beginToneI2s();
    toneCodecReady = toneI2sReady && initToneCodec();
    if (toneI2sReady && toneCodecReady) {
      ++toneRecoveries; toneRecoveryAttempts = 0;
      toneWorker.pauseRequested.store(false);
      Serial.println("[audio] recovered; ready for next notification");
    }
    return;
  }
  bool enable = toneTaskHandle && toneI2sReady && toneCodecReady && toneVolumePercent && toneWorker.ampWanted.load();
  if (enable != toneSpeakerEnabled) toneSetSpeakerEnabled(enable);
}

static void initToneAudio() {
  if (CYD_AUDIO_ENABLE_PIN >= 0) pinMode(CYD_AUDIO_ENABLE_PIN, OUTPUT);
  toneSetSpeakerEnabled(false);
  toneWorker.volume.store(toneVolumePercent);
  toneI2sReady = beginToneI2s();
  toneCodecReady = toneI2sReady && initToneCodec();
  toneRequestQueue = xQueueCreate(2, sizeof(ToneRequest));
  if (toneRequestQueue && xTaskCreate(toneAudioTask, "notify-audio", 6144, nullptr, 2, &toneTaskHandle) != pdPASS)
    toneTaskHandle = nullptr;
  if (!toneTaskHandle && toneRequestQueue) { vQueueDelete(toneRequestQueue); toneRequestQueue = nullptr; }
  if (!toneI2sReady || !toneCodecReady) requestToneRecovery(false);
  if (!toneTaskHandle) { toneI2sReady = toneCodecReady = false; toneSetSpeakerEnabled(false); }
  Serial.printf("[audio] data GPIO%d, I2S=%s codec=%s worker=%s volume=%u%% unity gain\n",
                CYD_AUDIO_DOUT_PIN, toneI2sReady ? "ready" : "fault", toneCodecReady ? "ready" : "fault",
                toneTaskHandle ? "ready" : "fault", toneVolumePercent);
  updateToneVolumeLabel();
}

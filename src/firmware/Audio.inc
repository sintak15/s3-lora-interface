// Internal implementation module; included once by the main sketch.
static const char TONE_STYLE_OPTIONS[] = "Low Bell\nChime\nTwo Step\nBeacon\nPager\nBell\nSustain\nRaw";
static const char TONE_BOOST_OPTIONS[] = "0 dB\n+3 dB\n+6 dB\n+9 dB\n+12 dB\nMax";

static const ToneSegment TONE_PATTERN_BWOMP[] = {
  {0, 120}, {980, 220, 780}, {0, 80}, {620, 420, 480}, {0, 180}
};
static const ToneSegment TONE_PATTERN_CHIME[] = {
  {0, 120}, {880, 300, 1047}, {0, 90}, {1175, 420, 880}, {0, 190}
};
static const ToneSegment TONE_PATTERN_DOUBLE[] = {
  {0, 120}, {784, 280, 880}, {0, 120}, {988, 360, 880}, {0, 200}
};
static const ToneSegment TONE_PATTERN_PULSE[] = {
  {0, 120}, {740, 220}, {0, 100}, {740, 220}, {0, 100}, {988, 300}, {0, 200}
};
static const ToneSegment TONE_PATTERN_ALERT[] = {
  {0, 120}, {1047, 320}, {0, 90}, {1047, 320}, {0, 90}, {784, 480, 659}, {0, 220}
};
static const ToneSegment TONE_PATTERN_LOUD[] = {
  {0, 130}, {880, 360, 1047}, {0, 100}, {1175, 460, 988}, {0, 100}, {988, 520, 784}, {0, 240}
};
static const ToneSegment TONE_PATTERN_SUSTAIN[] = {
  {0, 160}, {520, 2600}, {0, 850}
};
static const ToneSegment TONE_PATTERN_RAW[] = {
  {1000, 3000}
};

static uint8_t clampToneBoost(uint8_t level) {
  return level <= 5 ? level : 2;
}

static const char* toneBoostName(uint8_t level) {
  switch (clampToneBoost(level)) {
    case 1: return "+3 dB";
    case 2: return "+6 dB";
    case 3: return "+9 dB";
    case 4: return "+12 dB";
    case 5: return "Max";
    case 0:
    default:
      return "0 dB";
  }
}

static uint8_t toneBoostTargetRegister() {
  switch (clampToneBoost(toneBoostLevel)) {
    case 1: return 0xC5;
    case 2: return 0xCB;
    case 3: return 0xD1;
    case 4: return 0xD7;
    case 5: return 0xFF;
    case 0:
    default:
      return 0xBF;
  }
}

static uint8_t tonePcmDrivePercent() {
  switch (clampToneBoost(toneBoostLevel)) {
    case 1: return 100;
    case 2: return 100;
    case 3: return 100;
    case 4: return 100;
    case 5: return 100;
    case 0:
    default:
      return 100;
  }
}

static ToneStyle clampToneStyle(uint8_t style) {
  return style < TONE_STYLE_COUNT ? (ToneStyle)style : TONE_STYLE_BWOMP;
}

static const char* toneStyleName(ToneStyle style) {
  switch (style) {
    case TONE_STYLE_CHIME: return "Chime";
    case TONE_STYLE_DOUBLE: return "Two Step";
    case TONE_STYLE_PULSE: return "Beacon";
    case TONE_STYLE_ALERT: return "Pager";
    case TONE_STYLE_LOUD: return "Bell";
    case TONE_STYLE_SUSTAIN: return "Sustain";
    case TONE_STYLE_RAW: return "Raw";
    case TONE_STYLE_BWOMP:
    default:
      return "Low Bell";
  }
}

static ToneStyle toneStyleForKind(ToneKind kind) {
  switch (kind) {
    case TONE_DIRECT: return toneDirectStyle;
    case TONE_TEST: return toneMessageStyle;
    case TONE_MESSAGE: return toneMessageStyle;
    default: return TONE_STYLE_BWOMP;
  }
}

static const ToneSegment* tonePatternFor(ToneStyle style, size_t& count) {
  switch (style) {
    case TONE_STYLE_CHIME:
      count = sizeof(TONE_PATTERN_CHIME) / sizeof(TONE_PATTERN_CHIME[0]);
      return TONE_PATTERN_CHIME;
    case TONE_STYLE_DOUBLE:
      count = sizeof(TONE_PATTERN_DOUBLE) / sizeof(TONE_PATTERN_DOUBLE[0]);
      return TONE_PATTERN_DOUBLE;
    case TONE_STYLE_PULSE:
      count = sizeof(TONE_PATTERN_PULSE) / sizeof(TONE_PATTERN_PULSE[0]);
      return TONE_PATTERN_PULSE;
    case TONE_STYLE_ALERT:
      count = sizeof(TONE_PATTERN_ALERT) / sizeof(TONE_PATTERN_ALERT[0]);
      return TONE_PATTERN_ALERT;
    case TONE_STYLE_LOUD:
      count = sizeof(TONE_PATTERN_LOUD) / sizeof(TONE_PATTERN_LOUD[0]);
      return TONE_PATTERN_LOUD;
    case TONE_STYLE_SUSTAIN:
      count = sizeof(TONE_PATTERN_SUSTAIN) / sizeof(TONE_PATTERN_SUSTAIN[0]);
      return TONE_PATTERN_SUSTAIN;
    case TONE_STYLE_RAW:
      count = sizeof(TONE_PATTERN_RAW) / sizeof(TONE_PATTERN_RAW[0]);
      return TONE_PATTERN_RAW;
    case TONE_STYLE_BWOMP:
    default:
      count = sizeof(TONE_PATTERN_BWOMP) / sizeof(TONE_PATTERN_BWOMP[0]);
      return TONE_PATTERN_BWOMP;
  }
}

static uint8_t toneCodecVolumeRegister() {
  uint8_t pct = constrain((int)toneVolumePercent, 0, 100);
  if (pct == 0) return 0x00;
  return toneBoostTargetRegister();
}

static float toneCodecGainDb() {
  uint8_t reg = toneCodecVolumeRegister();
  if (reg == 0x00) return -95.5f;
  if (reg == 0x01) return -90.5f;
  return ((int)reg - 0xBF) * 0.5f;
}

static bool toneCodecWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission((uint8_t)CYD_AUDIO_CODEC_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static uint8_t toneCodecRead(uint8_t reg, uint8_t fallback) {
  Wire.beginTransmission((uint8_t)CYD_AUDIO_CODEC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return fallback;
  if (Wire.requestFrom((uint8_t)CYD_AUDIO_CODEC_ADDR, (uint8_t)1) != 1) return fallback;
  return Wire.read();
}

static int toneCodecReadInt(uint8_t reg) {
  if (!toneCodecReady) return -1;
  return (int)toneCodecRead(reg, 0xFF);
}

static uint8_t clampToneCodecProfile(uint8_t profile) {
  return profile < TONE_CODEC_PROFILE_COUNT ? profile : TONE_CODEC_PROFILE_REF_HP;
}

static const char* toneCodecProfileName(uint8_t profile) {
  switch (clampToneCodecProfile(profile)) {
    case TONE_CODEC_PROFILE_DRIVER: return "Driver";
    case TONE_CODEC_PROFILE_REF_LINE: return "Ref Line";
    case TONE_CODEC_PROFILE_LEGACY: return "Legacy";
    case TONE_CODEC_PROFILE_REF_HP:
    default:
      return "Ref HP";
  }
}

static uint8_t toneProfileReg0A(uint8_t profile) {
  (void)profile;
  return 0x4C;
}

static uint8_t toneProfileReg12(uint8_t profile) {
  switch (clampToneCodecProfile(profile)) {
    case TONE_CODEC_PROFILE_REF_HP:
    case TONE_CODEC_PROFILE_REF_LINE:
      return 0x01;
    case TONE_CODEC_PROFILE_DRIVER:
    case TONE_CODEC_PROFILE_LEGACY:
    default:
      return 0x00;
  }
}

static uint8_t toneProfileReg13(uint8_t profile) {
  return clampToneCodecProfile(profile) == TONE_CODEC_PROFILE_REF_LINE ? 0x00 : 0x10;
}

static uint8_t toneProfileReg15(uint8_t profile) {
  return clampToneCodecProfile(profile) == TONE_CODEC_PROFILE_LEGACY ? 0x00 : 0x40;
}

static uint8_t toneProfileReg37(uint8_t profile) {
  return clampToneCodecProfile(profile) == TONE_CODEC_PROFILE_LEGACY ? 0x08 : 0x48;
}

static bool writeToneCodecOutputProfileRaw() {
  const uint8_t profile = clampToneCodecProfile(toneCodecProfile);
  bool ok = true;
  ok &= toneCodecWrite(0x0A, toneProfileReg0A(profile));
  ok &= toneCodecWrite(0x17, 0xBF);
  ok &= toneCodecWrite(0x0E, 0x02);
  ok &= toneCodecWrite(0x12, toneProfileReg12(profile));
  ok &= toneCodecWrite(0x14, 0x1A);
  ok &= toneCodecWrite(0x0D, 0x01);
  ok &= toneCodecWrite(0x15, 0x40);
  ok &= toneCodecWrite(0x37, toneProfileReg37(profile));
  ok &= toneCodecWrite(0x45, 0x00);
  return ok;
}

static bool applyToneCodecOutputProfile() {
  if (!toneCodecReady) return false;
  return writeToneCodecOutputProfileRaw();
}

static bool initToneCodec() {
  Wire.beginTransmission((uint8_t)CYD_AUDIO_CODEC_ADDR);
  if (Wire.endTransmission() != 0) {
    toneCodecInitFailures++;
    return false;
  }

  bool ok = true;
  ok &= toneCodecWrite(0x01, 0x30);
  ok &= toneCodecWrite(0x02, 0x00);
  ok &= toneCodecWrite(0x03, 0x10);
  ok &= toneCodecWrite(0x16, 0x24);
  ok &= toneCodecWrite(0x04, 0x10);
  ok &= toneCodecWrite(0x05, 0x00);
  ok &= toneCodecWrite(0x0B, 0x00);
  ok &= toneCodecWrite(0x0C, 0x00);
  ok &= toneCodecWrite(0x10, 0x1F);
  ok &= toneCodecWrite(0x11, 0x7F);
  ok &= toneCodecWrite(0x00, 0x80);
  delay(12);
  uint8_t resetReg = toneCodecRead(0x00, 0x80);
  resetReg &= (uint8_t)~0x40;
  ok &= toneCodecWrite(0x00, resetReg);
  ok &= toneCodecWrite(0x01, 0x3F);
  uint8_t clkReg = toneCodecRead(0x01, 0x3F);
  clkReg &= (uint8_t)~0x80;
  ok &= toneCodecWrite(0x01, clkReg);
  ok &= toneCodecWrite(0x02, 0x00);
  ok &= toneCodecWrite(0x03, 0x10);
  ok &= toneCodecWrite(0x04, 0x10);
  ok &= toneCodecWrite(0x05, 0x00);
  ok &= toneCodecWrite(0x06, 0x03);
  ok &= toneCodecWrite(0x07, 0x00);
  ok &= toneCodecWrite(0x08, 0xFF);
  ok &= toneCodecWrite(0x09, 0x0C);
  ok &= toneCodecWrite(0x13, 0x10);
  ok &= toneCodecWrite(0x1B, 0x0A);
  ok &= toneCodecWrite(0x1C, 0x6A);
  ok &= toneCodecWrite(0x0D, 0x01);
  ok &= toneCodecWrite(0x0E, 0x02);
  ok &= writeToneCodecOutputProfileRaw();
  ok &= toneCodecWrite(0x32, toneCodecVolumeRegister());

  uint8_t muteReg = toneCodecRead(0x31, 0x00);
  if (toneVolumePercent == 0) muteReg |= 0x60;
  else muteReg &= (uint8_t)~0x60;
  ok &= toneCodecWrite(0x31, muteReg);
  ok &= toneCodecWrite(0x00, 0x80);
  if (!ok) toneCodecInitFailures++;
  return ok;
}

static void toneSetSpeakerEnabled(bool enabled) {
  if (CYD_AUDIO_ENABLE_PIN < 0) return;
  toneSpeakerEnabled = enabled;
  const int disabledLevel = (CYD_AUDIO_ENABLE_LEVEL == LOW) ? HIGH : LOW;
  digitalWrite(CYD_AUDIO_ENABLE_PIN, enabled ? CYD_AUDIO_ENABLE_LEVEL : disabledLevel);
}

static void applyToneVolume() {
  toneVolumePercent = constrain((int)toneVolumePercent, 0, 100);
  if (toneCodecReady) {
    toneCodecWrite(0x32, toneCodecVolumeRegister());
    uint8_t muteReg = toneCodecRead(0x31, 0x00);
    if (toneVolumePercent == 0) muteReg |= 0x60;
    else muteReg &= (uint8_t)~0x60;
    toneCodecWrite(0x31, muteReg);
  }
  if (toneVolumePercent == 0) {
    toneActive = false;
    pendingToneKind = TONE_NONE;
    toneSpeakerHoldUntilMs = 0;
    toneSetSpeakerEnabled(false);
  }
}

static const char* toneAudioStateLabel() {
  if (!toneI2sReady) return "not ready";
  return toneCodecReady ? "I2S + codec" : "I2S";
}

static void updateToneVolumeLabel() {
  if (!lblToneVolume) return;
  char text[176];
  snprintf(text, sizeof(text),
           "Volume: %u%%  Boost: %s\nCodec: %s  %.1f dB  %s\nMsg: %s  DM: %s",
           toneVolumePercent,
           toneBoostName(toneBoostLevel),
           toneAudioStateLabel(),
           toneCodecGainDb(),
           toneCodecProfileName(toneCodecProfile),
           toneStyleName(toneMessageStyle),
           toneStyleName(toneDirectStyle));
  lv_label_set_text(lblToneVolume, text);
}

static void toneVolumeSliderEvent(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  toneVolumePercent = constrain((int)lv_slider_get_value(slider), 0, 100);
  prefs.putUChar("toneVol", toneVolumePercent);
  applyToneVolume();
  updateToneVolumeLabel();
}

static void updateToneDropdowns() {
  if (dropdownToneBoost && lv_dropdown_get_selected(dropdownToneBoost) != (uint16_t)toneBoostLevel) {
    lv_dropdown_set_selected(dropdownToneBoost, (uint16_t)toneBoostLevel);
  }
  if (dropdownToneMessage && lv_dropdown_get_selected(dropdownToneMessage) != (uint16_t)toneMessageStyle) {
    lv_dropdown_set_selected(dropdownToneMessage, (uint16_t)toneMessageStyle);
  }
  if (dropdownToneDirect && lv_dropdown_get_selected(dropdownToneDirect) != (uint16_t)toneDirectStyle) {
    lv_dropdown_set_selected(dropdownToneDirect, (uint16_t)toneDirectStyle);
  }
}

static void toneBoostDropdownEvent(lv_event_t* e) {
  lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
  toneBoostLevel = clampToneBoost((uint8_t)lv_dropdown_get_selected(dropdown));
  prefs.putUChar("toneBoost", toneBoostLevel);
  applyToneVolume();
  updateToneVolumeLabel();
}

static void toneMessageDropdownEvent(lv_event_t* e) {
  lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
  toneMessageStyle = clampToneStyle((uint8_t)lv_dropdown_get_selected(dropdown));
  prefs.putUChar("toneMsg", (uint8_t)toneMessageStyle);
  updateToneVolumeLabel();
}

static void toneDirectDropdownEvent(lv_event_t* e) {
  lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
  toneDirectStyle = clampToneStyle((uint8_t)lv_dropdown_get_selected(dropdown));
  prefs.putUChar("toneDir", (uint8_t)toneDirectStyle);
  updateToneVolumeLabel();
}

static bool startToneSegment(uint8_t segmentIndex) {
  size_t count = 0;
  const ToneSegment* pattern = tonePatternFor(activeToneStyle, count);
  if (!pattern || segmentIndex >= count) return false;
  activeToneSegment = segmentIndex;
  activeToneFramesRemaining = ((uint32_t)pattern[segmentIndex].durationMs * TONE_SAMPLE_RATE) / 1000U;
  if (activeToneFramesRemaining == 0) activeToneFramesRemaining = 1;
  activeToneSegmentTotalFrames = activeToneFramesRemaining;
  tonePhase = 0.0f;
  return true;
}

static void startTone(ToneKind kind) {
  if (!toneI2sReady || toneVolumePercent == 0 || kind == TONE_NONE) return;
  activeToneKind = kind;
  activeToneStyle = toneStyleForKind(kind);
  toneActive = startToneSegment(0);
  if (!toneActive) {
    activeToneKind = TONE_NONE;
    return;
  }
  lastToneMs = millis();
  toneSpeakerHoldUntilMs = 0;
  toneSetSpeakerEnabled(true);
}

static void finishTone() {
  toneActive = false;
  activeToneKind = TONE_NONE;
  activeToneFramesRemaining = 0;
  activeToneSegmentTotalFrames = 0;
  if (pendingToneKind != TONE_NONE) {
    ToneKind next = pendingToneKind;
    pendingToneKind = TONE_NONE;
    startTone(next);
    return;
  }
  toneSpeakerHoldUntilMs = millis() + TONE_SPEAKER_HOLD_MS;
}

static void queueTone(ToneKind kind, bool replace) {
  if (!toneI2sReady || toneVolumePercent == 0 || kind == TONE_NONE) return;
  if (replace) {
    pendingToneKind = TONE_NONE;
    startTone(kind);
    return;
  }
  if (toneActive) {
    if (kind == TONE_DIRECT || pendingToneKind == TONE_NONE) pendingToneKind = kind;
    return;
  }
  startTone(kind);
}

static void queueToneStyle(ToneStyle style, bool replace) {
  if (!toneI2sReady || toneVolumePercent == 0) return;
  ToneStyle previousMessageStyle = toneMessageStyle;
  toneMessageStyle = clampToneStyle((uint8_t)style);
  queueTone(TONE_TEST, replace);
  toneMessageStyle = previousMessageStyle;
}

static int8_t toneStyleIndexFromText(String value) {
  value.trim();
  value.toLowerCase();
  if (!value.length()) return -1;
  if (isDigit(value[0])) {
    int index = value.toInt();
    return (index >= 0 && index < TONE_STYLE_COUNT) ? (int8_t)index : -1;
  }
  if (value == "low bell" || value == "bwomp") return TONE_STYLE_BWOMP;
  if (value == "chime") return TONE_STYLE_CHIME;
  if (value == "two step" || value == "double") return TONE_STYLE_DOUBLE;
  if (value == "beacon" || value == "pulse") return TONE_STYLE_PULSE;
  if (value == "pager" || value == "alert") return TONE_STYLE_ALERT;
  if (value == "bell" || value == "loud") return TONE_STYLE_LOUD;
  if (value == "sustain" || value == "carrier" || value == "test") return TONE_STYLE_SUSTAIN;
  if (value == "raw" || value == "square" || value == "tone") return TONE_STYLE_RAW;
  return -1;
}

static int8_t toneBoostIndexFromText(String value) {
  value.trim();
  value.toLowerCase();
  if (!value.length()) return -1;
  if (value == "max") return 5;
  value.replace("db", "");
  value.replace("+", "");
  value.trim();
  int db = value.toInt();
  switch (db) {
    case 0: return 0;
    case 3: return 1;
    case 6: return 2;
    case 9: return 3;
    case 12: return 4;
    case 5: return 5;
    default:
      return -1;
  }
}

static int8_t toneCodecProfileIndexFromText(String value) {
  value.trim();
  value.toLowerCase();
  if (!value.length()) return -1;
  if (isDigit(value[0])) {
    int index = value.toInt();
    return (index >= 0 && index < TONE_CODEC_PROFILE_COUNT) ? (int8_t)index : -1;
  }
  value.replace("-", "");
  value.replace("_", "");
  value.replace(" ", "");
  if (value == "driver" || value == "default" || value == "official") return TONE_CODEC_PROFILE_DRIVER;
  if (value == "refhp" || value == "hp" || value == "speaker" || value == "reference") return TONE_CODEC_PROFILE_REF_HP;
  if (value == "refline" || value == "line" || value == "lineout") return TONE_CODEC_PROFILE_REF_LINE;
  if (value == "legacy" || value == "old") return TONE_CODEC_PROFILE_LEGACY;
  return -1;
}

static bool parseByteArg(String value, uint8_t& out) {
  value.trim();
  if (!value.length()) return false;
  const char* start = value.c_str();
  char* end = nullptr;
  unsigned long parsed = strtoul(start, &end, 0);
  if (end == start || *end != '\0' || parsed > 0xFF) return false;
  out = (uint8_t)parsed;
  return true;
}

static String buildToneStatusJson() {
  String json = "{";
  json += "\"toneVolume\":" + String(toneVolumePercent) + ",";
  json += "\"toneBoost\":\"" + String(toneBoostName(toneBoostLevel)) + "\",";
  json += "\"toneDrive\":" + String(tonePcmDrivePercent()) + ",";
  json += "\"toneCodecProfile\":\"" + String(toneCodecProfileName(toneCodecProfile)) + "\",";
  json += "\"toneCodecProfileIndex\":" + String(clampToneCodecProfile(toneCodecProfile)) + ",";
  json += "\"toneMessage\":\"" + String(toneStyleName(toneMessageStyle)) + "\",";
  json += "\"toneDirect\":\"" + String(toneStyleName(toneDirectStyle)) + "\",";
  json += "\"toneCodecReg\":" + String(toneCodecVolumeRegister()) + ",";
  json += "\"toneCodecGainDb\":" + String(toneCodecGainDb(), 1) + ",";
  json += "\"toneAudioReady\":" + String(toneI2sReady ? "true" : "false") + ",";
  json += "\"toneCodecReady\":" + String(toneCodecReady ? "true" : "false") + ",";
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
  json += "\"toneLastI2sError\":" + String(toneLastI2sError);
  json += "}";
  return json;
}

static void serviceToneAudio() {
  if (!toneActive && toneSpeakerEnabled && toneSpeakerHoldUntilMs && (int32_t)(millis() - toneSpeakerHoldUntilMs) >= 0) {
    toneSpeakerHoldUntilMs = 0;
    toneSetSpeakerEnabled(false);
  }
  if (!toneI2sReady || !toneActive) return;
  if (toneVolumePercent == 0) {
    finishTone();
    return;
  }

  size_t count = 0;
  const ToneSegment* pattern = tonePatternFor(activeToneStyle, count);
  if (!pattern || activeToneSegment >= count) {
    finishTone();
    return;
  }

  uint32_t framesToWrite = activeToneFramesRemaining;
  if (framesToWrite > TONE_FRAMES_PER_SERVICE) framesToWrite = TONE_FRAMES_PER_SERVICE;

  int16_t frames[TONE_FRAMES_PER_SERVICE * 2];
  const ToneSegment& segment = pattern[activeToneSegment];
  const int32_t amplitude = (int32_t)map(constrain((int)toneVolumePercent, 0, 100), 0, 100, 0, 32700)
                              * (int32_t)tonePcmDrivePercent() / 100;
  const float startFrequency = (float)segment.frequency;
  const float endFrequency = (float)(segment.endFrequency ? segment.endFrequency : segment.frequency);
  for (uint32_t i = 0; i < framesToWrite; i++) {
    int16_t sample = 0;
    if (segment.frequency) {
      uint32_t frameIndex = activeToneSegmentTotalFrames > activeToneFramesRemaining
                              ? activeToneSegmentTotalFrames - activeToneFramesRemaining + i
                              : i;
      float progress = activeToneSegmentTotalFrames > 1
                         ? (float)frameIndex / (float)(activeToneSegmentTotalFrames - 1)
                         : 1.0f;
      if (progress > 1.0f) progress = 1.0f;
      float frequency = startFrequency + (endFrequency - startFrequency) * progress;
      uint32_t frameIndexInSegment = activeToneSegmentTotalFrames > activeToneFramesRemaining
                                       ? activeToneSegmentTotalFrames - activeToneFramesRemaining + i
                                       : i;
      uint32_t framesLeftInSegment = activeToneSegmentTotalFrames > frameIndexInSegment
                                       ? activeToneSegmentTotalFrames - frameIndexInSegment
                                       : 0;
      uint32_t rampFrames = min<uint32_t>(TONE_SAMPLE_RATE / 12U, activeToneSegmentTotalFrames / 3U);
      if (rampFrames == 0) rampFrames = 1;
      float envelope = 1.0f;
      if (frameIndexInSegment < rampFrames) envelope = (float)frameIndexInSegment / (float)rampFrames;
      if (framesLeftInSegment < rampFrames) envelope = min(envelope, (float)framesLeftInSegment / (float)rampFrames);
      if (envelope < 0.0f) envelope = 0.0f;
      envelope = envelope * envelope * (3.0f - (2.0f * envelope));

      if (activeToneStyle == TONE_STYLE_RAW) {
        float wrapped = fmodf(tonePhase, 6.28318530718f);
        sample = wrapped < 3.14159265359f ? amplitude : (int16_t)-amplitude;
      } else {
        float sine = sinf(tonePhase);
        float shaped = sine + (0.08f * sinf(tonePhase * 2.0f)) + (0.05f * sinf(tonePhase * 3.0f));
        shaped *= 0.96f;
        if (shaped > 1.0f) shaped = 1.0f;
        if (shaped < -1.0f) shaped = -1.0f;
        sample = (int16_t)(shaped * amplitude * envelope);
      }
      float phaseStep = (6.28318530718f * frequency) / (float)TONE_SAMPLE_RATE;
      tonePhase += phaseStep;
      if (tonePhase >= 6.28318530718f) tonePhase -= 6.28318530718f;
    }
    frames[i * 2] = sample;
    frames[i * 2 + 1] = sample;
  }

  const size_t requestedBytes = (size_t)framesToWrite * 2U * sizeof(int16_t);
  const size_t writtenBytes = toneI2s.write((const uint8_t*)frames, requestedBytes);
  toneWriteCalls++;
  toneLastWriteRequested = requestedBytes;
  toneLastWriteBytes = writtenBytes;
  toneBytesWritten += (uint32_t)writtenBytes;
  toneLastI2sError = toneI2s.lastError();
  if (writtenBytes == 0) toneWriteFailures++;
  if (writtenBytes != requestedBytes) toneShortWrites++;
  activeToneFramesRemaining -= framesToWrite;
  if (activeToneFramesRemaining > 0) return;
  if (!startToneSegment(activeToneSegment + 1)) finishTone();
}

static void initToneAudio() {
  if (CYD_AUDIO_ENABLE_PIN >= 0) {
    pinMode(CYD_AUDIO_ENABLE_PIN, OUTPUT);
    toneSetSpeakerEnabled(false);
  }

  toneI2s.setPins(CYD_AUDIO_BCLK_PIN, CYD_AUDIO_LRCLK_PIN, CYD_AUDIO_DOUT_PIN, -1, CYD_AUDIO_MCLK_PIN);
  toneI2sReady = toneI2s.begin(I2S_MODE_STD,
                               TONE_SAMPLE_RATE,
                               I2S_DATA_BIT_WIDTH_16BIT,
                               I2S_SLOT_MODE_STEREO);
  toneCodecReady = initToneCodec();
  applyToneVolume();

  char line[128];
  snprintf(line, sizeof(line),
           "[audio] CYD speaker %s, codec %s, volume %u%%\n",
           toneI2sReady ? "ready" : "not ready",
           toneCodecReady ? "ready" : "not detected",
           toneVolumePercent);
  appendLine(eventLog, LOG_SIZE, line);
  Serial.print(line);
  updateToneVolumeLabel();
}

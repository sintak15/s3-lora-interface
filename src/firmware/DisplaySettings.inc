// Internal implementation module; included once by the main sketch.
static void applyBacklight() {
  uint8_t pct = constrain(backlightPercent, 10, 100);
  uint32_t duty = map(pct, 0, 100, 0, 255);
  if (backlightPwmReady) {
    ledcWrite(TFT_BL, duty);
  } else {
    analogWrite(TFT_BL, duty);
  }
}

static void initBacklight() {
  Serial.printf("[boot] Backlight pin %d configuring...\n", TFT_BL);
  pinMode(TFT_BL, OUTPUT);
  backlightPwmReady = ledcAttach(TFT_BL, 5000, 8);
  Serial.printf("[boot] Backlight PWM ready: %d\n", backlightPwmReady);
  applyBacklight();
}

static void applyFontMode() {
  if (readableTextMode) {
    UI_FONT_SMALL = LV_FONT_DEFAULT;
    UI_FONT_DEFAULT = UI_FONT_READABLE;
  } else {
    UI_FONT_SMALL = &comic_neue_12;
    UI_FONT_DEFAULT = &comic_neue_14;
  }
}

static void backlightSliderEvent(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  backlightPercent = constrain((int)lv_slider_get_value(slider), 10, 100);
  applyBacklight();
}

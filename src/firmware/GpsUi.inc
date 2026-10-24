// Internal GPS Config / Adv pages; included once by Ui.inc.
static GpsConfig::Changes gpsConfigDraft;
static uint32_t gpsConfigUiCompletedOperation = 0;
static const uint16_t GPS_CONFIG_PERIODS[] = {0, 5000, 2000, 1000, 500, 200};
static const uint8_t GPS_CONFIG_MODELS[] = {GpsConfig::KEEP, 0, 2, 3, 4, 5, 6, 7, 8};
static const uint8_t GPS_CONFIG_SBAS[] = {GpsConfig::KEEP, 1, 0};
static const uint8_t GPS_CONFIG_POWER[] = {GpsConfig::KEEP, 0, 4, 1};

static void syncGpsConfigDraft() {
  const uint16_t values[] = {gpsConfigDraft.periodMs, gpsConfigDraft.model, gpsConfigDraft.sbas,
                            gpsConfigDraft.power, (uint16_t)gpsConfigDraft.profile};
  for (uint8_t control = 0; control < 5; ++control) {
    if (!gpsConfigDropdowns[control]) continue;
    uint16_t selected = 0;
    if (control == 0) { for (uint8_t i = 0; i < 6; ++i) if (GPS_CONFIG_PERIODS[i] == values[0]) selected = i; }
    else if (control == 1) { for (uint8_t i = 0; i < 9; ++i) if (GPS_CONFIG_MODELS[i] == values[1]) selected = i; }
    else if (control == 2) { for (uint8_t i = 0; i < 3; ++i) if (GPS_CONFIG_SBAS[i] == values[2]) selected = i; }
    else if (control == 3) { for (uint8_t i = 0; i < 4; ++i) if (GPS_CONFIG_POWER[i] == values[3]) selected = i; }
    else selected = values[4];
    lv_dropdown_set_selected(gpsConfigDropdowns[control], selected);
  }
}

static void gpsConfigChanged(lv_event_t*) {
  gpsConfigDraft.periodMs = GPS_CONFIG_PERIODS[lv_dropdown_get_selected(gpsConfigDropdowns[0])];
  gpsConfigDraft.model = GPS_CONFIG_MODELS[lv_dropdown_get_selected(gpsConfigDropdowns[1])];
  gpsConfigDraft.sbas = GPS_CONFIG_SBAS[lv_dropdown_get_selected(gpsConfigDropdowns[2])];
  gpsConfigDraft.power = GPS_CONFIG_POWER[lv_dropdown_get_selected(gpsConfigDropdowns[3])];
  gpsConfigDraft.profile = (GpsConfig::Profile)lv_dropdown_get_selected(gpsConfigDropdowns[4]);
  forcePageRefresh = true;
}

static void applyGpsConfigDraft(lv_event_t*) {
  gpsConfig.apply(gpsConfigDraft, millis());
  forcePageRefresh = true;
}

static void saveGpsConfig(lv_event_t*) {
  if (!gpsConfigDraft.any()) gpsConfig.save(millis());
  forcePageRefresh = true;
}

static void openGpsConfig() {
  showPage(pageGpsConfig);
  if (!gpsConfig.busy() && gpsConfig.current.known != GpsConfig::ALL_KNOWN) gpsConfig.read(millis());
}

static void setGpsControlEnabled(lv_obj_t* control, bool enabled) {
  if (!control) return;
  if (enabled) lv_obj_clear_state(control, LV_STATE_DISABLED);
  else lv_obj_add_state(control, LV_STATE_DISABLED);
}

static void refreshGpsConfigUi() {
  if (currentPage != pageGpsConfig && currentPage != pageGpsAdvanced) return;
  using namespace GpsConfig;
  if (!gpsConfig.busy() && gpsConfig.serial != gpsConfigUiCompletedOperation) {
    gpsConfigUiCompletedOperation = gpsConfig.serial;
    if (gpsConfig.result == Result::Applied) { gpsConfigDraft = {}; syncGpsConfigDraft(); }
  }
  char status[256];
  const Error validation = gpsConfig.validate(gpsConfigDraft);
  if (gpsConfig.busy()) {
    snprintf(status, sizeof(status), "%s %u/%u\n%s",
             gpsConfig.operation == Operation::Read ? "Reading" : gpsConfig.operation == Operation::Save ? "Saving" : "Applying",
             (unsigned)gpsConfig.index + 1, (unsigned)gpsConfig.count, fieldName(gpsConfig.field()));
  } else if (gpsConfig.result == Result::Failed) {
    snprintf(status, sizeof(status), "%s: %s\nRead receiver before retrying.",
             fieldName(gpsConfig.errorField), errorText(gpsConfig.error));
  } else if (gpsConfigDraft.any()) {
    snprintf(status, sizeof(status), "Unapplied changes\n%s", validation == Error::None ?
             "Apply checks them against the receiver." : errorText(validation));
  } else if (gpsConfig.result == Result::Saved) strlcpy(status, "Receiver accepted save.\nRetention depends on receiver memory / backup power.", sizeof(status));
  else if (gpsConfig.result == Result::Applied) strlcpy(status, "Settings applied and read back.\nSave to retain in receiver memory.", sizeof(status));
  else if (gpsConfig.result == Result::PartialRead) strlcpy(status, "Some receiver settings unavailable.\nOnly confirmed fields can be changed.", sizeof(status));
  else strlcpy(status, gpsConfig.current.coreKnown() ? "Receiver settings loaded.\nChoose only the fields to change." : "Read receiver settings to begin.", sizeof(status));
  for (auto* label : gpsConfigStatusLabels) if (label) lv_label_set_text(label, status);

  const auto& current = gpsConfig.current;
  char text[96];
  if (current.has(Field::Rate)) snprintf(text, sizeof(text), "Update rate\nCurrent: %.1f Hz", 1000.0 / current.period());
  else strlcpy(text, "Update rate\nCurrent: unknown", sizeof(text));
  setReadoutText(gpsConfigCurrentLabels[0], text);
  snprintf(text, sizeof(text), "Navigation mode\nCurrent: %s", current.has(Field::Navigation) ? modelName(current.navigation[2]) : "unknown");
  setReadoutText(gpsConfigCurrentLabels[1], text);
  snprintf(text, sizeof(text), "SBAS assistance\nCurrent: %s", current.has(Field::Sbas) ? ((current.sbas[0] & 1) ? "Enabled" : "Disabled") : "unknown");
  setReadoutText(gpsConfigCurrentLabels[2], text);
  snprintf(text, sizeof(text), "Power mode\nCurrent: %s", current.has(Field::Power) ? powerName(current.power[1]) : "unknown");
  setReadoutText(gpsConfigCurrentLabels[3], text);
  snprintf(text, sizeof(text), "NMEA output\nCurrent: %s", profileName(current.profile()));
  setReadoutText(gpsConfigCurrentLabels[4], text);
  for (auto* dropdown : gpsConfigDropdowns) setGpsControlEnabled(dropdown, !gpsConfig.busy());
  setGpsControlEnabled(btnGpsConfigRead, !gpsConfig.busy());
  for (auto* button : gpsConfigApplyButtons) setGpsControlEnabled(button, !gpsConfig.busy() && validation == Error::None);
  for (auto* button : gpsConfigSaveButtons)
    setGpsControlEnabled(button, !gpsConfig.busy() && gpsConfig.canSave && !gpsConfigDraft.any());
  if (switchGpsRelay) {
    if (gpsRelay.enabled) lv_obj_add_state(switchGpsRelay, LV_STATE_CHECKED);
    else lv_obj_clear_state(switchGpsRelay, LV_STATE_CHECKED);
    setGpsControlEnabled(switchGpsRelay, !gpsRelaySetupBusy());
  }
  setGpsControlEnabled(btnGpsRelaySetup, !gpsRelaySetupBusy());
  if (lblGpsRelay) {
    char shareText[200];
    snprintf(shareText, sizeof(shareText), "%s\nQueued: %lu  Accepted: %lu",
             gpsRelaySetupBusy() ? "Checking Heltec settings..." :
             gpsRelaySetupError()[0] ? gpsRelaySetupError() : GpsRelay::stateText(gpsRelayState()),
             (unsigned long)gpsRelay.queued, (unsigned long)gpsRelay.accepted);
    setReadoutText(lblGpsRelay, shareText);
  }
}

static lv_obj_t* makeGpsConfigButton(lv_obj_t* parent, const char* title, int x, int y, int width, lv_event_cb_t callback) {
  lv_obj_t* button = makeSmallButton(parent, title, x, y, width, callback);
  lv_obj_set_height(button, 44);
  lv_obj_set_style_bg_opa(button, LV_OPA_40, LV_STATE_DISABLED);
  return button;
}

static void makeGpsConfigInput(lv_obj_t* parent, uint8_t index, const char* options, int y) {
  gpsConfigCurrentLabels[index] = lv_label_create(parent);
  styleBoundedLabel(gpsConfigCurrentLabels[index], lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(gpsConfigCurrentLabels[index], LV_ALIGN_TOP_LEFT, 2, y);
  gpsConfigDropdowns[index] = lv_dropdown_create(parent);
  lv_dropdown_set_options(gpsConfigDropdowns[index], options);
  lv_obj_set_size(gpsConfigDropdowns[index], SCREEN_W - 38, 40);
  lv_obj_align(gpsConfigDropdowns[index], LV_ALIGN_TOP_MID, 0, y + 42);
  styleDarkObject(gpsConfigDropdowns[index], COLOR_INPUT, COLOR_TEXT);
  styleDarkBorder(gpsConfigDropdowns[index], COLOR_BORDER);
  lv_obj_set_style_radius(gpsConfigDropdowns[index], 6, 0);
  lv_obj_add_event_cb(gpsConfigDropdowns[index], gpsConfigChanged, LV_EVENT_VALUE_CHANGED, nullptr);
}

static void buildGpsConfigUi() {
  makePageTitle(pageGpsConfig, "GPS Config");
  makePageScrollable(pageGpsConfig);
  lv_obj_t* panel = makePanel(pageGpsConfig);
  lv_obj_set_size(panel, SCREEN_W - 12, 690);
  lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 26);
  gpsConfigStatusLabels[0] = lv_label_create(panel);
  styleBoundedLabel(gpsConfigStatusLabels[0], lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(gpsConfigStatusLabels[0], LV_ALIGN_TOP_LEFT, 2, 0);
  btnGpsConfigRead = makeGpsConfigButton(panel, "Read receiver", 2, 86, SCREEN_W - 38, [](lv_event_t*) {
    gpsConfig.read(millis()); forcePageRefresh = true;
  });
  makeGpsConfigInput(panel, 0, "Keep receiver\n0.2 Hz (every 5 s)\n0.5 Hz (every 2 s)\n1 Hz (recommended)\n2 Hz\n5 Hz", 142);
  makeGpsConfigInput(panel, 1, "Keep receiver\nPortable\nStationary\nWalking\nDriving\nAt sea\nAirborne <1g\nAirborne <2g\nAirborne <4g", 232);
  makeGpsConfigInput(panel, 2, "Keep receiver\nEnabled\nDisabled", 322);
  gpsConfigApplyButtons[0] = makeGpsConfigButton(panel, "Apply", 2, 416, 98, applyGpsConfigDraft);
  gpsConfigSaveButtons[0] = makeGpsConfigButton(panel, "Save", 106, 416, 98, saveGpsConfig);
  makeGpsConfigButton(panel, "Adv", 2, 474, SCREEN_W - 38, [](lv_event_t*) { showPage(pageGpsAdvanced); });
  makeGpsConfigButton(panel, "Clear edits", 2, 530, SCREEN_W - 38, [](lv_event_t*) {
    gpsConfigDraft = {}; syncGpsConfigDraft(); forcePageRefresh = true;
  });
  lv_obj_t* hint = lv_label_create(panel);
  lv_label_set_text(hint, "Apply changes first. Save keeps the confirmed settings in receiver memory.");
  styleBoundedLabel(hint, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 2, 590);

  makePageTitle(pageGpsAdvanced, "GPS Adv");
  makePageScrollable(pageGpsAdvanced);
  panel = makePanel(pageGpsAdvanced);
  lv_obj_set_size(panel, SCREEN_W - 12, 825);
  lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 26);
  gpsConfigStatusLabels[1] = lv_label_create(panel);
  styleBoundedLabel(gpsConfigStatusLabels[1], lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(gpsConfigStatusLabels[1], LV_ALIGN_TOP_LEFT, 2, 0);
  makeGpsConfigInput(panel, 3, "Keep receiver\nMaximum performance\nEco\nPower save", 90);
  makeGpsConfigInput(panel, 4, "Keep receiver\nNormal\nCompact", 180);
  lv_obj_t* advancedHint = lv_label_create(panel);
  lv_label_set_text(advancedHint, "Compact keeps position updates and reduces satellite-detail traffic. Needed above 1 Hz.\nEco / Power save: use 1 Hz.\nApply includes edits on both pages.");
  styleBoundedLabel(advancedHint, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(advancedHint, LV_ALIGN_TOP_LEFT, 2, 278);
  gpsConfigApplyButtons[1] = makeGpsConfigButton(panel, "Apply", 2, 408, 98, applyGpsConfigDraft);
  gpsConfigSaveButtons[1] = makeGpsConfigButton(panel, "Save", 106, 408, 98, saveGpsConfig);
  lv_obj_t* shareTitle = lv_label_create(panel);
  lv_label_set_text(shareTitle, "Share GPS\nto Heltec");
  styleBoundedLabel(shareTitle, 132, COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(shareTitle, LV_ALIGN_TOP_LEFT, 2, 480);
  switchGpsRelay = lv_switch_create(panel);
  lv_obj_set_size(switchGpsRelay, 52, 30);
  lv_obj_align(switchGpsRelay, LV_ALIGN_TOP_RIGHT, -2, 480);
  lv_obj_add_event_cb(switchGpsRelay, [](lv_event_t* event) {
    setGpsRelayEnabled(lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED));
    forcePageRefresh = true;
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lblGpsRelay = lv_label_create(panel);
  styleBoundedLabel(lblGpsRelay, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblGpsRelay, LV_ALIGN_TOP_LEFT, 2, 530);
  btnGpsRelaySetup = makeGpsConfigButton(panel, "Use CYD GPS", 2, 640, SCREEN_W - 38, [](lv_event_t*) {
    prepareGpsRelay(); forcePageRefresh = true;
  });
  lv_obj_t* shareHint = lv_label_create(panel);
  lv_label_set_text(shareHint, "Setup switches Heltec to external GPS, verifies settings, then enables sharing. Heltec may restart; retry setup if needed. Remove the GPS-to-Heltec wire after verification. Sharing needs the CYD running.");
  styleBoundedLabel(shareHint, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(shareHint, LV_ALIGN_TOP_LEFT, 2, 700);
  syncGpsConfigDraft();
}

#ifdef FIRMWARE_RUN_TESTS
static void runHomeUiTests() {
  auto savedBattery = localBattery;
  auto savedGps = gpsStats;
  bool savedSeen = radioResponseSeen;
  uint32_t savedResponse = lastRadioResponseMs;
  uint16_t savedUnread = unreadPublic, savedNodes = nodeCount;
  char savedPreview[sizeof(previewLatest)]; memcpy(savedPreview, previewLatest, sizeof(previewLatest));
  auto* savedPage = currentPage;
  lv_obj_t* savedNavigation[8]; memcpy(savedNavigation, navigationStack, sizeof(savedNavigation));
  uint8_t savedDepth = navigationDepth;
  bool all = true;
  auto check = [&all](const char* name, bool ok) { Serial.printf("[test-home] %s: %s\n", name, ok ? "PASS" : "FAIL"); all &= ok; };
  showPage(pageLauncher, false);
  radioResponseSeen = false; localBattery.gaugePresent = false; refreshDashboardLabels();
  check("Waiting and missing gauge remain explicit", strcmp(lv_label_get_text(lblHomeLink), "Waiting") == 0 && strcmp(lv_label_get_text(lblHomePower), "--") == 0 && lv_obj_has_flag(homePowerBar, LV_OBJ_FLAG_HIDDEN));
  radioResponseSeen = true; lastRadioResponseMs = millis();
  localBattery.gaugePresent = true; localBattery.charging = true; localBattery.percent = 100;
  refreshDashboardLabels();
  check("Connected and charging states", strcmp(lv_label_get_text(lblHomeLink), "Connected") == 0 && strcmp(lv_label_get_text(lblHomePower), "100%") == 0 && strcmp(lv_label_get_text(lblHomePowerDetail), "Battery: Increasing") == 0 && lv_bar_get_value(homePowerBar) == 100);
  lastRadioResponseMs = millis() - 100000; localBattery.charging = false; localBattery.batteryLow = localBattery.batteryCritical = true; localBattery.percent = 5;
  gpsStats.valid = true; gpsStats.lastUpdateMs = millis() - 31000;
  unreadPublic = 999; nodeCount = MAX_NODES;
  memset(previewLatest, 'W', sizeof(previewLatest)-1); previewLatest[sizeof(previewLatest)-1] = 0;
  refreshDashboardLabels();
  check("Stale radio, critical battery and stale GPS", strcmp(lv_label_get_text(lblHomeLink), "No response") == 0 && strcmp(lv_label_get_text(lblHomePowerDetail), "Charge soon") == 0 && strcmp(lv_label_get_text(lblHomeGps), "GPS ready") != 0);
  lv_obj_update_layout(pageLauncher);
  bool bounds = true, uniform = true; unsigned cards = 0;
  lv_area_t page; lv_obj_get_coords(pageLauncher, &page);
  for (uint32_t i=0; i<lv_obj_get_child_cnt(pageLauncher); ++i) {
    auto* card = lv_obj_get_child(pageLauncher, i);
    if (!lv_obj_has_flag(card, LV_OBJ_FLAG_CLICKABLE)) continue;
    ++cards; lv_area_t area; lv_obj_get_coords(card, &area);
    bounds &= lv_obj_get_height(card) >= 44 && area.x1 >= page.x1 && area.x2 <= page.x2 && area.y1 >= page.y1 && area.y2 <= page.y2;
    uniform &= lv_obj_get_style_bg_color(card, LV_PART_MAIN).full == lv_color_hex(0x102238).full;
    for (uint32_t j=0; j<lv_obj_get_child_cnt(card); ++j) {
      lv_area_t child; lv_obj_get_coords(lv_obj_get_child(card,j), &child);
      bounds &= child.x1 >= area.x1 && child.x2 <= area.x2 && child.y1 >= area.y1 && child.y2 <= area.y2;
    }
  }
  check("Four uniform cards with bounded long text", cards == 4 && bounds && uniform);
  lv_obj_t* labels[] = {lblHomeLink, lblHomePower, lblHomeNodes, lblHomeMessages};
  nodeCount = savedNodes;
  lv_obj_t* pages[] = {pageSystemRadio, pageBattery, pageNodes, pageOperate};
  bool navigation = true;
  for (unsigned i=0;i<4;++i) {
    navigationDepth = 0; showPage(pageLauncher,false);
    lv_event_send(lv_obj_get_parent(labels[i]), LV_EVENT_CLICKED, nullptr);
    navigation &= currentPage == pages[i];
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    navigation &= currentPage == pageLauncher;
  }
  check("Each card opens its detail and Back returns home", navigation);
  localBattery = savedBattery; gpsStats = savedGps; radioResponseSeen = savedSeen; lastRadioResponseMs = savedResponse;
  unreadPublic = savedUnread; nodeCount = savedNodes; memcpy(previewLatest, savedPreview, sizeof(previewLatest));
  memcpy(navigationStack, savedNavigation, sizeof(savedNavigation)); navigationDepth = savedDepth;
  showPage(savedPage, false); refreshDashboardLabels();
  Serial.printf("[test-home] complete: %s\n", all ? "PASS" : "FAIL");
}

// Capture the actual pixels sent to the TFT, without enabling LVGL snapshots or
// allocating a permanent framebuffer. Small acknowledged chunks avoid USB loss.
static bool handleScreenCaptureCommand(const char* command) {
  if (strcmp(command, "screen-close") == 0) {
    if (testScreenPixels) heap_caps_free(testScreenPixels);
    testScreenPixels = nullptr; Serial.println("[SCREEN] CLOSED"); return true;
  }
  if (strcmp(command, "screen-home") == 0) {
    if (landscapeKeyboardOpen || usbTileUploadActive() || usbSdExportActive()) { Serial.println("[SCREEN] ERROR busy"); return true; }
    if (!testScreenPixels) testScreenPixels = (lv_color_t*)heap_caps_malloc(SCREEN_W * SCREEN_H * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!testScreenPixels) { Serial.println("[SCREEN] ERROR allocation"); return true; }
    showPage(pageLauncher, false); refreshScreenUi();
    testScreenCapturedMs = 0; lv_obj_invalidate(mainScreen); lv_refr_now(nullptr); testScreenCapturedMs = millis();
    Serial.printf("[SCREEN] READY %d %d %u\n", SCREEN_W, SCREEN_H, (unsigned)(SCREEN_W * SCREEN_H * sizeof(lv_color_t))); return true;
  }
  if (strncmp(command, "screen-read ", 12) == 0) {
    uint32_t offset = 0;
    const size_t bytes = SCREEN_W * SCREEN_H * sizeof(lv_color_t);
    if (!testScreenPixels || !tileNumber(command + 12, offset) || offset >= bytes) { Serial.println("[SCREEN] ERROR offset"); return true; }
    size_t count = min((size_t)768, bytes - (size_t)offset), encodedLength = 0;
    auto* source = (uint8_t*)testScreenPixels + offset;
    unsigned char encoded[1025];
    if (mbedtls_base64_encode(encoded, sizeof(encoded), &encodedLength, source, count) != 0) { Serial.println("[SCREEN] ERROR encoding"); return true; }
    encoded[encodedLength] = 0; testScreenCapturedMs = millis();
    Serial.printf("[SCREEN] DATA %lu %08lx %s\n", (unsigned long)offset, (unsigned long)(TileUpload::crcUpdate(0xffffffffU, source, count)^0xffffffffU), encoded); return true;
  }
  return false;
}
#endif

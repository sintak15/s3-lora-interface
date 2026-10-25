// Verification-only inspection of the real LVGL tree and TFT-bound pixels.
// Test values and font changes are never saved to preferences or SD.
namespace UiLayoutTests {
struct Page { const char* name; lv_obj_t* object; };
static Page page(unsigned index) {
  const Page pages[] = {
    {"home", pageLauncher}, {"menu", pageFunctions}, {"messages", pageOperate},
    {"health", pageDiagnose}, {"nodes", pageNodes}, {"node-detail", pageNodeDetail},
    {"mesh-health", pageMeshHealth}, {"packets", pagePacketInspector}, {"delivery", pageTxHistory},
    {"public", pagePublicChat}, {"secondary", pagePrivateChat}, {"direct", pageDirectChat},
    {"message-test", pageMessageDiag}, {"map", pageGps}, {"coverage", pageMapCoverage},
    {"system", pageSystem}, {"interface", pageSystemInterface}, {"serial", pageSystemSerial},
    {"radio", pageSystemRadio}, {"gps", pageSystemGps}, {"gps-config", pageGpsConfig},
    {"gps-advanced", pageGpsAdvanced}, {"wifi", pageWifi}, {"wifi-stats", pageWifiStats},
    {"wifi-connect", pageWifiLocal}, {"wifi-scan", pageWifiScan}, {"display", pageBacklight},
    {"sound", pageSound}, {"battery", pageBattery}, {"power-logs", pagePowerLogs}
  };
  return index < sizeof(pages) / sizeof(pages[0]) ? pages[index] : Page{nullptr, nullptr};
}
static int index(lv_obj_t* object) {
  for (unsigned i = 0; page(i).name; ++i) if (page(i).object == object) return i;
  return 0;
}
static bool ancestor(lv_obj_t* parent, lv_obj_t* child) {
  for (auto* at = lv_obj_get_parent(child); at; at = lv_obj_get_parent(at)) if (at == parent) return true;
  return false;
}
static bool leaf(lv_obj_t* object) {
    return lv_obj_check_type(object, &lv_img_class) || lv_obj_check_type(object, &lv_label_class) || lv_obj_check_type(object, &lv_btn_class) ||
         lv_obj_check_type(object, &lv_textarea_class) || lv_obj_check_type(object, &lv_dropdown_class) ||
         lv_obj_check_type(object, &lv_checkbox_class) || lv_obj_check_type(object, &lv_switch_class) ||
         lv_obj_check_type(object, &lv_slider_class) || lv_obj_check_type(object, &lv_bar_class);
}
static const char* text(lv_obj_t* object) {
  if (lv_obj_check_type(object, &lv_label_class)) return lv_label_get_text(object);
  if (lv_obj_check_type(object, &lv_checkbox_class)) return lv_checkbox_get_text(object);
  if (lv_obj_check_type(object, &lv_textarea_class)) return "[text panel]";
  if (lv_obj_check_type(object, &lv_dropdown_class)) return "[dropdown]";
  if (lv_obj_check_type(object, &lv_slider_class)) return "[slider]";
  if (lv_obj_check_type(object, &lv_bar_class)) return "[bar]";
  if (lv_obj_get_child_cnt(object)) return text(lv_obj_get_child(object, 0));
  return "[control]";
}
struct Entry { lv_obj_t* object; lv_area_t box; };
struct Audit {
  Entry* entries;
  lv_obj_t* root = nullptr;
  unsigned count = 0, labels = 0, issues = 0;
  String details;
  void issue(const char* kind, lv_obj_t* a, lv_obj_t* b = nullptr) {
    ++issues;
    if (issues > 40) return;
    auto describe = [](lv_obj_t* object) {
      lv_area_t area; lv_obj_get_coords(object, &area);
      String value = "{\"text\":\"" + jsonEscape(String(text(object)).substring(0, 60).c_str()) + "\",\"box\":[";
      return value + area.x1 + "," + area.y1 + "," + area.x2 + "," + area.y2 + "]}";
    };
    if (details.length()) details += ",";
    details += String("{\"kind\":\"") + kind + "\",\"a\":" + describe(a);
    if (b) details += ",\"b\":" + describe(b);
    details += "}";
  }
  void collect(lv_obj_t* object) {
    if (!object || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN) || object == mapLayer) return;
    const bool label = lv_obj_check_type(object, &lv_label_class);
    if (label && !lv_label_get_text(object)[0]) return;
    lv_area_t area; lv_obj_get_coords(object, &area);
    auto* parent = lv_obj_get_parent(object);
    if (parent && !lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE)) {
      lv_area_t container; lv_obj_get_coords(parent, &container);
      if (area.x1 < container.x1 || area.x2 > container.x2 || area.y1 < container.y1 || area.y2 > container.y2)
        issue("outside-parent", object, parent);
    }
    if (label) {
      ++labels;
      if (lv_obj_get_height(object) < lv_obj_get_style_text_font(object, 0)->line_height)
        issue("line-height", object);
    }
    if (leaf(object)) {
      // Nested lists clip their rows. Root pages are audited in full so that
      // controls below the fold are checked too, without comparing them to nav.
      for (auto* at = parent; at && at != root; at = lv_obj_get_parent(at)) {
        if (!lv_obj_has_flag(at, LV_OBJ_FLAG_SCROLLABLE)) continue;
        lv_area_t viewport; lv_obj_get_coords(at, &viewport);
        area.x1 = max(area.x1, viewport.x1); area.y1 = max(area.y1, viewport.y1);
        area.x2 = min(area.x2, viewport.x2); area.y2 = min(area.y2, viewport.y2);
      }
      if (area.x1 <= area.x2 && area.y1 <= area.y2) {
        if (count < 2048) entries[count++] = {object, area};
        else issue("audit-capacity", object);
      }
    }
    // A text area's label is intentionally larger than its scrolling viewport.
    if (lv_obj_check_type(object, &lv_textarea_class)) return;
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(object); ++i) collect(lv_obj_get_child(object, i));
  }
  void overlaps(unsigned first = 0) {
    for (unsigned i = first; i < count; ++i) for (unsigned j = i + 1; j < count; ++j) {
      const auto& a = entries[i]; const auto& b = entries[j];
      if (a.box.x1 > b.box.x2 || b.box.x1 > a.box.x2 || a.box.y1 > b.box.y2 || b.box.y1 > a.box.y2) continue;
      if (ancestor(a.object, b.object) || ancestor(b.object, a.object)) continue;
      issue("overlap", a.object, b.object);
    }
  }
};

static void handle() {
  if (!requireWebAuth()) return;
  if (uiReadInteractionActive() || landscapeKeyboardOpen || usbTileUploadActive() || usbSdExportActive()) {
    server.send(409, "text/plain", "UI busy"); return;
  }
  unsigned target = 0;
  const String requested = server.arg("screen");
  if (requested == "regressions") {
    bool home = runHomeUiTests(), ui = runUiSmokeTests();
    server.send(200, "application/json", String("{\"home\":") + (home ? "true" : "false") +
                ",\"ui\":" + (ui ? "true" : "false") + "}");
    return;
  }
  while (page(target).name && requested != page(target).name) ++target;
  if (!page(target).name) { server.send(400, "text/plain", "Unknown screen"); return; }
  auto* entries = (Entry*)heap_caps_calloc(2048, sizeof(Entry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!entries) { server.send(503, "text/plain", "No inspection memory"); return; }
  const bool savedReadable = readableTextMode;
  int savedPage = index(currentPage), savedNavigation[8];
  uint8_t savedDepth = navigationDepth;
  for (unsigned i = 0; i < savedDepth; ++i) savedNavigation[i] = index(navigationStack[i]);
  auto savedBattery = localBattery; auto savedRuntime = batteryRuntime; auto savedGps = gpsStats;
  const auto savedListView = nodeListViewMode;
  uint32_t savedSelected = selectedNodeNum, savedResponse = lastRadioResponseMs;
  bool savedSeen = radioResponseSeen;
  uint16_t savedNodeCount = nodeCount, savedUnread = unreadPublic;
  uint16_t savedFamilyUnread = unreadFamily, savedDirectUnread = unreadDirect;
  char savedPreview[sizeof(previewLatest)]; memcpy(savedPreview, previewLatest, sizeof(previewLatest));
  setReadableTextMode(server.arg("font") == "readable", false);
  ensureWifiLocalPage(); ensureWifiScanPage();
  const String variant = server.arg("case");
  if (requested == "home" && variant != "real") {
    localBattery.gaugePresent = true; localBattery.percent = 49; localBattery.chargeRateValid = true;
    localBattery.charging = false; localBattery.batteryLow = localBattery.batteryCritical = false;
    batteryRuntime.restore(10); batteryRuntime.sample(millis(), 49, true, false);
    if (variant == "charging") { localBattery.charging = true; localBattery.percent = 100; }
    if (variant == "unknown") localBattery.chargeRateValid = false;
    if (variant == "missing") localBattery.gaugePresent = false;
    if (variant == "critical") { localBattery.batteryCritical = true; localBattery.percent = 5; }
    if (variant == "long") {
      radioResponseSeen = true; lastRadioResponseMs = millis() - 100000;
      unreadPublic = 999; nodeCount = MAX_NODES;
      memset(previewLatest, 'W', sizeof(previewLatest) - 1); previewLatest[sizeof(previewLatest) - 1] = 0;
    }
  }
  if (requested == "nodes") nodeListViewMode = variant == "detail" ? NODE_LIST_VIEW_DETAILED : NODE_LIST_VIEW_COMPACT;
  if (requested == "node-detail" && nodeCount) selectedNodeNum = nodes[0].num;
  showPage(page(target).object, false);
  forcePageRefresh = true; refreshScreenUi(); lv_obj_update_layout(mainScreen);
  Audit audit{entries};
  audit.root = page(target).object; audit.collect(audit.root); audit.overlaps();
  unsigned bars = audit.count;
  audit.root = statusBar; audit.collect(statusBar);
  audit.root = navBar; audit.collect(navBar); audit.overlaps(bars);
  if (requested == "home") {
    for (auto* label : {lblHomePowerDetail, lblHomePower, lblHomeNodes, lblHomeLink, lblHomeUnread}) {
      lv_point_t natural;
      lv_txt_get_size(&natural, lv_label_get_text(label), lv_obj_get_style_text_font(label, 0),
                      lv_obj_get_style_text_letter_space(label, 0), lv_obj_get_style_text_line_space(label, 0),
                      LV_COORD_MAX, LV_TEXT_FLAG_NONE);
      if (natural.x > lv_obj_get_width(label)) audit.issue("essential-text-truncated", label);
    }
  }
  String result = "{\"screen\":\"" + requested + "\",\"font\":\"" + server.arg("font") +
                  "\",\"case\":\"" + jsonEscape(variant.c_str()) + "\",\"objects\":" + audit.count +
                  ",\"labels\":" + audit.labels + ",\"issueCount\":" + audit.issues + ",\"issues\":[" + audit.details + "]";
  if (server.arg("capture") == "1") {
    if (!testScreenPixels) testScreenPixels = (lv_color_t*)heap_caps_malloc(SCREEN_W * SCREEN_H * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (testScreenPixels) {
      testScreenCapturedMs = 0; lv_obj_invalidate(mainScreen); lv_refr_now(nullptr); testScreenCapturedMs = millis();
      uint32_t crc = TileUpload::crcUpdate(0xffffffffU, (uint8_t*)testScreenPixels, SCREEN_W * SCREEN_H * sizeof(lv_color_t)) ^ 0xffffffffU;
      result += ",\"frameCrc\":" + String(crc);
    }
  }
  result += "}";
  heap_caps_free(entries);
  localBattery = savedBattery; batteryRuntime = savedRuntime; gpsStats = savedGps;
  radioResponseSeen = savedSeen; lastRadioResponseMs = savedResponse; nodeCount = savedNodeCount;
  unreadPublic = savedUnread; selectedNodeNum = savedSelected; nodeListViewMode = savedListView;
  unreadFamily = savedFamilyUnread; unreadDirect = savedDirectUnread;
  memcpy(previewLatest, savedPreview, sizeof(previewLatest));
  setReadableTextMode(savedReadable, false);
  for (unsigned i = 0; i < savedDepth; ++i) navigationStack[i] = page(savedNavigation[i]).object;
  navigationDepth = savedDepth; showPage(page(savedPage).object, false);
  forcePageRefresh = true; refreshScreenUi();
  server.send(200, "application/json", result);
}
static void frame() {
  if (!requireWebAuth()) return;
  if (!testScreenPixels) { server.send(404, "text/plain", "No captured frame"); return; }
  size_t bytes = SCREEN_W * SCREEN_H * sizeof(lv_color_t), offset = 0;
  server.setContentLength(bytes); server.send(200, "application/octet-stream", "");
  auto client = server.client();
  while (offset < bytes && client.connected()) {
    size_t sent = client.write((uint8_t*)testScreenPixels + offset, min((size_t)1024, bytes - offset));
    if (!sent) break;
    offset += sent; yield();
  }
  heap_caps_free(testScreenPixels); testScreenPixels = nullptr;
}
} // namespace UiLayoutTests

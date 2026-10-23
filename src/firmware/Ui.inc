// Internal implementation module; included once by the main sketch.
static void lvFlush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* colors) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)colors, w * h, true);
  tft.endWrite();
#ifdef FIRMWARE_RUN_TESTS
  if (testScreenPixels && testScreenCapturedMs == 0 && area->x1 >= 0 && area->y1 >= 0 && area->x2 < SCREEN_W && area->y2 < SCREEN_H) {
    for (uint32_t row = 0; row < h; ++row) memcpy(testScreenPixels + (area->y1 + row) * SCREEN_W + area->x1, colors + row * w, w * sizeof(lv_color_t));
  }
#endif
  lv_disp_flush_ready(disp);
}

static bool readTouch(uint16_t& x, uint16_t& y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)5) != 5) return false;

  uint8_t touches = Wire.read();
  uint8_t xh = Wire.read();
  uint8_t xl = Wire.read();
  uint8_t yh = Wire.read();
  uint8_t yl = Wire.read();
  if (touches == 0 || touches > 2) return false;

  uint16_t rawX = constrain(((uint16_t)(xh & 0x0F) << 8) | xl, 0, SCREEN_W - 1);
  uint16_t rawY = constrain(((uint16_t)(yh & 0x0F) << 8) | yl, 0, SCREEN_H - 1);
  if (landscapeKeyboardOpen) {
    x = rawY;
    y = SCREEN_W - 1 - rawX;
  } else {
    x = rawX;
    y = rawY;
  }
  return true;
}

static void lvTouchRead(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  uint16_t x = 0;
  uint16_t y = 0;
  if (readTouch(x, y)) {
    touchSamples++;
    lastTouchMs = millis();
    lastTouchX = x;
    lastTouchY = y;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void styleDarkObject(lv_obj_t* obj, uint32_t bg, uint32_t text) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_color(obj, lv_color_hex(text), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

static void styleDarkBorder(lv_obj_t* obj, uint32_t color) {
  lv_obj_set_style_border_color(obj, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
}

static void styleDarkTextArea(lv_obj_t* ta) {
  styleDarkObject(ta, COLOR_INPUT, 0xE8FFF5);
  styleDarkBorder(ta, 0x2F705F);
  lv_obj_set_style_text_font(ta, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_line_space(ta, 1, 0);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_MUTED), LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0x16342C), LV_PART_SELECTED);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_TEXT), LV_PART_SELECTED);
}

static void styleBoundedLabel(lv_obj_t* label, lv_coord_t width, uint32_t color, lv_label_long_mode_t mode) {
  if (!label) return;
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, mode);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
}

static lv_obj_t* makePanel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  styleDarkObject(panel, COLOR_PANEL);
  styleDarkBorder(panel);
  lv_obj_set_style_radius(panel, 6, 0);
  lv_obj_set_style_pad_all(panel, 5, 0);
  return panel;
}

static lv_obj_t* makePage(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H - NAV_BAR_H - STATUS_BAR_H);
  lv_obj_align(page, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_H);
  styleDarkObject(page, COLOR_BG);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 5, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  return page;
}

static void makePageScrollable(lv_obj_t* page) {
  lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
}

static void buildStatusBar(lv_obj_t* screen) {
  statusBar = lv_obj_create(screen);
  lv_obj_set_size(statusBar, SCREEN_W, STATUS_BAR_H);
  lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);
  styleDarkObject(statusBar, 0x080D0B, COLOR_MUTED);
  lv_obj_set_style_border_color(statusBar, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_side(statusBar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(statusBar, 1, 0);
  lv_obj_set_style_pad_hor(statusBar, 6, 0);
  lv_obj_set_style_pad_ver(statusBar, 2, 0);
  lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);

  lblStatus = lv_label_create(statusBar);
  lv_label_set_text(lblStatus, "booting");
  lv_obj_set_width(lblStatus, SCREEN_W - 158);
  lv_label_set_long_mode(lblStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(lblStatus, UI_FONT_SMALL, 0);
  lv_obj_align(lblStatus, LV_ALIGN_LEFT_MID, 0, 0);

  lblRadioStatus = lv_label_create(statusBar);
  lv_label_set_text(lblRadioStatus, LV_SYMBOL_WIFI);
  lv_obj_set_width(lblRadioStatus, 28);
  lv_label_set_long_mode(lblRadioStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblRadioStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lblRadioStatus, lv_color_hex(0xFF5A5F), 0);
  lv_obj_set_style_text_font(lblRadioStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblRadioStatus, LV_ALIGN_RIGHT_MID, -126, 0);

  lblGpsStatus = lv_label_create(statusBar);
  lv_label_set_text(lblGpsStatus, LV_SYMBOL_GPS " 0");
  lv_obj_set_width(lblGpsStatus, 40);
  lv_label_set_long_mode(lblGpsStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblGpsStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lblGpsStatus, lv_color_hex(0xFF5A5F), 0);
  lv_obj_set_style_text_font(lblGpsStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblGpsStatus, LV_ALIGN_RIGHT_MID, -84, 0);

  lblBatteryStatus = lv_label_create(statusBar);
  lv_label_set_text(lblBatteryStatus, "Batt --%");
  lv_obj_set_width(lblBatteryStatus, 82);
  lv_label_set_long_mode(lblBatteryStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblBatteryStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_color(lblBatteryStatus, lv_color_hex(COLOR_ACCENT), 0);
  // LVGL symbols require the built-in symbol-capable font.
  lv_obj_set_style_text_font(lblBatteryStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblBatteryStatus, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void showPage(lv_obj_t* target, bool remember) {
  if (!target) return;
  bool leavingNodeDetailMap = (currentPage == pageGps || currentPage == pageMapCoverage) && target != pageGps && target != pageMapCoverage && mapNodeDetailViewActive;
  if (remember && currentPage && currentPage != target) {
    if (navigationDepth == 8) {
      memmove(navigationStack, navigationStack + 1, 7 * sizeof(navigationStack[0]));
      navigationDepth = 7;
    }
    navigationStack[navigationDepth++] = currentPage;
  }
  lv_obj_t* pages[] = {
    pageLauncher, pageFunctions, pageOperate, pageDiagnose, pageNodes, pageNodeDetail, pageMeshHealth, pagePacketInspector,
    pageTxHistory, pagePublicChat, pagePrivateChat, pageDirectChat, pageMessageDiag, pageGps, pageMapCoverage,
    pageSystem, pageSystemInterface, pageSystemSerial, pageSystemRadio, pageSystemGps,
    pageWifi, pageWifiStats, pageWifiLocal, pageWifiScan, pageBacklight, pageSound, pageBattery
  };
  for (lv_obj_t* page : pages) {
    if (!page) continue;
    if (page == target) lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  currentPage = target;
  if (btnNavBack && btnNavHome) {
    bool home = target == pageLauncher;
    lv_label_set_text(lv_obj_get_child(btnNavBack, 0), home ? "Menu" : "Back");
    lv_obj_set_width(btnNavBack, home ? 220 : 92);
    lv_obj_set_width(lv_obj_get_child(btnNavBack, 0), home ? 200 : 80);
    if (home) lv_obj_add_flag(btnNavHome, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(btnNavHome, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_radius(btnNavBack, home ? 10 : 6, 0);
    lv_obj_set_style_bg_color(btnNavBack, lv_color_hex(home ? 0x18362B : 0x111A18), 0);
    lv_obj_set_style_border_color(btnNavBack, lv_color_hex(home ? 0x345346 : 0x315B50), 0);
    lv_obj_set_style_text_font(lv_obj_get_child(btnNavBack, 0), home ? &lv_font_montserrat_14 : UI_FONT_SMALL, 0);
  }
  if (target == pagePublicChat) unreadPublic = 0;
  if (target == pagePrivateChat) unreadFamily = 0;
  if (target == pageDirectChat) unreadDirect = 0;
  if (leavingNodeDetailMap) {
    mapNodeDetailViewActive = false;
    mapFocusSelectedNode = false;
    mapNearbyMode = false;
    mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
    mapView.resume();
    mapCanvasCached = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
  }
  forcePageRefresh = true;
  refreshDashboardLabels();
  if (target == pageGps) {
    if (!mapNodeDetailViewActive) {
      mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
      mapFocusSelectedNode = false;
    }
    if (!mapFocusSelectedNode) mapNearbyMode = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
  }
}

static int activeMapPlotHeight() {
  return MAP_PLOT_H_NORMAL;
}

static void invalidateMapView() {
  mapCanvasCached = false;
  mapRenderPending = true;
  markUiDirty(UI_DIRTY_MAP);
  lastMapUiRefreshMs = 0;
  forcePageRefresh = true;
  if (mapLayer) lv_obj_set_pos(mapLayer, 0, 0);
  if (mapCanvas) lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
  for (auto* dot : mapDots) if (dot) lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
  if (mapScale) lv_obj_add_flag(mapScale, LV_OBJ_FLAG_HIDDEN);
}

static void updateMapControls() {
  if (!lblMapZoom) return;
  char text[24];
  snprintf(text, sizeof(text), "Zoom %d\nCoverage", mapView.zoom);
  lv_label_set_text(lblMapZoom, text);
  if (mapView.zoom <= MAP_TILE_MIN_ZOOM) lv_obj_add_state(btnMapZoomOut, LV_STATE_DISABLED);
  else lv_obj_clear_state(btnMapZoomOut, LV_STATE_DISABLED);
  if (mapView.zoom >= MAP_TILE_MAX_ZOOM) lv_obj_add_state(btnMapZoomIn, LV_STATE_DISABLED);
  else lv_obj_clear_state(btnMapZoomIn, LV_STATE_DISABLED);
  lv_label_set_text(lblMapFollow, mapView.follow ? "Follow on" : "Follow off");
  if (mapView.follow) lv_obj_add_state(btnMapFollow, LV_STATE_CHECKED);
  else lv_obj_clear_state(btnMapFollow, LV_STATE_CHECKED);
}

static void updateMapScale() {
  if (!mapScale) return;
  if (!mapCanvasCached) { lv_obj_add_flag(mapScale, LV_OBJ_FLAG_HIDDEN); return; }
  auto scale = MapNavigation::scale(cachedMapLat, cachedMapZoom);
  char text[20];
  if (scale.feet) snprintf(text, sizeof(text), "%u ft", scale.feet);
  else if (scale.miles == 0.25) strlcpy(text, "1/4 mi", sizeof(text));
  else if (scale.miles == 0.5) strlcpy(text, "1/2 mi", sizeof(text));
  else snprintf(text, sizeof(text), "%.0f mi", scale.miles);
  lv_label_set_text(lblMapScale, text);
  lv_obj_set_width(mapScaleBar, scale.pixels);
  lv_obj_set_x(mapScaleEnd, 4 + scale.pixels - 1);
  lv_obj_clear_flag(mapScale, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(mapScale);
}

static void setMapZoom(int zoom, bool save) {
  if (!mapView.setZoom(zoom)) return;
  mapDragging = mapDragMoved = false;
  if (save) { mapZoomSavePending = true; mapZoomSaveAtMs = millis() + 1000; }
  invalidateMapView();
  updateMapControls();
  refreshMapUi();
}

static void serviceMapNavigationSettings() {
  if (mapZoomSavePending && !mapNavigationTesting && (int32_t)(millis() - mapZoomSaveAtMs) >= 0) {
    prefs.putUChar("mapZoom", (uint8_t)mapView.zoom);
    mapZoomSavePending = false;
  }
}

static void setMapFollow(bool enabled) {
  if (enabled) {
    mapView.resume();
    mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
    mapNodeDetailViewActive = false;
    mapFocusSelectedNode = false;
  } else {
    if (mapCanvasCached) { mapView.center = {cachedMapLat, cachedMapLon}; mapView.hasCenter = true; }
    mapView.pause();
  }
  mapDragging = mapDragMoved = false;
  invalidateMapView();
  updateMapControls();
  refreshMapUi();
}

static void mapDragEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t* input = lv_indev_get_act();
  if (!input) return;
  lv_point_t point;
  lv_indev_get_point(input, &point);
  if (code == LV_EVENT_PRESSED) {
    mapDragging = mapView.hasCenter;
    mapDragMoved = false;
    mapDragStart = point;
    mapDragOrigin = mapCanvasCached ? MapNavigation::Point{cachedMapLat, cachedMapLon} : mapView.center;
    mapDragZoom = mapCanvasCached ? cachedMapZoom : mapView.zoom;
  } else if (code == LV_EVENT_PRESSING && mapDragging) {
    int dx = point.x - mapDragStart.x, dy = point.y - mapDragStart.y;
    if (abs(dx) >= 6 || abs(dy) >= 6) mapDragMoved = true;
    if (mapDragMoved && mapLayer) lv_obj_set_pos(mapLayer, dx, dy);
  } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && mapDragging) {
    mapDragging = false;
    if (mapDragMoved) {
      mapView.center = MapNavigation::pan(mapDragOrigin, mapDragZoom, point.x - mapDragStart.x, point.y - mapDragStart.y);
      mapView.hasCenter = mapView.manual = true;
      mapView.follow = false;
      ++mapView.revision;
      invalidateMapView();
      updateMapControls();
      refreshMapUi();
    } else if (mapLayer) lv_obj_set_pos(mapLayer, 0, 0);
    mapDragMoved = false;
  }
}

static void clearUiObjectPointers() {
  pageLauncher = nullptr;
  pageFunctions = nullptr;
  pageOperate = nullptr;
  pageDiagnose = nullptr;
  pageNodes = nullptr;
  pageNodeDetail = nullptr;
  pageMeshHealth = nullptr;
  pagePacketInspector = nullptr;
  pageTxHistory = nullptr;
  pagePublicChat = nullptr;
  pagePrivateChat = nullptr;
  pageDirectChat = nullptr;
  pageMessageDiag = nullptr;
  pageGps = nullptr;
  pageMapCoverage = nullptr;
  pageSystem = nullptr;
  pageSystemInterface = nullptr;
  pageSystemSerial = nullptr;
  pageSystemRadio = nullptr;
  pageSystemGps = nullptr;
  pageWifi = nullptr;
  pageWifiStats = nullptr;
  pageWifiLocal = nullptr;
  pageWifiScan = nullptr;
  pageBacklight = nullptr;
  pageSound = nullptr;
  pageBattery = nullptr;
  currentPage = nullptr;
  navigationDepth = 0;
  memset(navigationStack, 0, sizeof(navigationStack));
  statusBar = nullptr;
  navBar = nullptr;
  lblStatus = nullptr;
  lblRadioStatus = nullptr;
  lblGpsStatus = nullptr;
  lblBatteryStatus = nullptr;
  lblStats = nullptr;
  lblHomeLink = nullptr;
  lblHomeMessages = nullptr;
  lblHomeMessageTitle = nullptr;
  lblHomeNodes = nullptr;
  btnNavBack = nullptr;
  btnNavHome = nullptr;
  lblHomeGps = nullptr;
  lblHomePower = nullptr;
  lblHomeLinkDetail = lblHomePowerDetail = lblHomeUnread = homePowerBar = homeLinkIcon = nullptr;
  lblTilePublic = nullptr;
  lblTilePrivate = nullptr;
  lblMsgPublic = nullptr;
  lblMsgFamily = nullptr;
  lblMsgDirect = nullptr;
  lblPublicChatTitle = nullptr;
  lblPrivateChatTitle = nullptr;
  lblPublicChannelStats = nullptr;
  lblPrivateChannelStats = nullptr;
  lblPrivateChannelHint = nullptr;
  listNodes = nullptr;
  lblNodeListMode = nullptr;
  taNodeDetail = nullptr;
  btnNodeFavorite = nullptr;
  lblNodeFavorite = nullptr;
  nodeDetailLastText[0] = '\0';
  nodeDetailRenderedNode = 0;
  nodeDetailScrollTopPending = false;
  lblMeshHealth = nullptr;
  taPacketInspector = nullptr;
  taTxHistory = nullptr;
  btnTxRetry = nullptr;
  lblSystemInterface = nullptr;
  lblSystemSerial = nullptr;
  lblSystemRadio = nullptr;
  lblWifiState = nullptr;
  lblWifiStats = nullptr;
  lblWifiScanStatus = nullptr;
  swWifiEnabled = nullptr;
  swWifiApMode = nullptr;
  taWifiSsid = nullptr;
  taWifiPass = nullptr;
  listWifiScan = nullptr;
  sliderBacklight = nullptr;
  lblBacklight = nullptr;
  sliderToneVolume = nullptr;
  lblToneVolume = nullptr;
  dropdownToneMessage = nullptr;
  dropdownToneDirect = nullptr;
  swReadableText = nullptr;
  lblTextMode = nullptr;
  lblBatteryStats = nullptr;
  lblGpsStats = nullptr;
  lblMapStats = nullptr;
  lblMapZoom = btnMapZoomOut = btnMapZoomIn = btnMapLocate = btnMapFollow = lblMapFollow = nullptr;
  btnMapCoverage = lblMapCoverage = lblMapCoverageSummary = nullptr;
  memset(lblMapCoverageRows, 0, sizeof(lblMapCoverageRows));
  mapLayer = lblMapNotice = mapScale = lblMapScale = mapScaleBar = mapScaleEnd = nullptr;
  mapDragging = mapDragMoved = false;
  mapCanvasCached = false;
  ++mapView.revision;
  mapPlot = nullptr;
  mapCanvas = nullptr;
  mapNodeDetailViewActive = false;
  mapFocusSelectedNode = false;
  mapNearbyMode = false;
  mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
  memset(mapDots, 0, sizeof(mapDots));
  taPublicChat = nullptr;
  taFamilyChat = nullptr;
  taDirectChat = nullptr;
  taScreenLog = nullptr;
  taScreenNodes = nullptr;
  taPublicInput = nullptr;
  taFamilyInput = nullptr;
  taDirectTo = nullptr;
  taDirectInput = nullptr;
  dropdownMsgDiagA = nullptr;
  dropdownMsgDiagB = nullptr;
  taMsgDiagMessage = nullptr;
  lblMsgDiagStatus = nullptr;
  lblMsgDiagDetail = nullptr;
  activeChatInput = nullptr;
  keyboard = nullptr;
  mainScreen = nullptr;
  wifiLocalPageBuilt = false;
  wifiScanPageBuilt = false;
}

static lv_obj_t* makeActionButton(lv_obj_t* parent, const char* text, int y, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, UI_ACTION_W, UI_ACTION_H);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, UI_ACTION_W - 20);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeSmallButton(lv_obj_t* parent, const char* text, int x, int y, int w, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, 22);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, w - 8);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeSystemTile(lv_obj_t* parent, const char* text, int col, int row, lv_event_cb_t cb) {
  const int tileW = UI_TILE_W;
  const int tileH = UI_TILE_H;
  const int gap = UI_GAP;
  const int x = 6 + col * (tileW + gap);
  const int y = 24 + row * (tileH + gap);
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, tileW, tileH);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, tileW - 12);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, UI_FONT_DEFAULT, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

// Home actions use separate title/subtitle labels and fixed, finger-sized bounds.
static lv_obj_t* makeHomeAction(lv_obj_t* parent, const char* title, const char* detail,
                               int x, int y, int w, int h, bool primary, lv_event_cb_t cb) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_size(button, w, h);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(button, primary ? COLOR_ACTION : COLOR_PANEL);
  styleDarkBorder(button, primary ? COLOR_ACTION : COLOR_BORDER);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_pad_all(button, 8, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(primary ? 0x68FFC0 : 0x24483E), LV_STATE_PRESSED);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* heading = lv_label_create(button);
  lv_obj_set_width(heading, w - 18);
  lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
  lv_label_set_text(heading, title);
  lv_obj_set_style_text_font(heading, UI_FONT_READABLE, 0);
  lv_obj_set_style_text_color(heading, lv_color_hex(primary ? 0x001B12 : COLOR_TEXT), 0);
  lv_obj_align(heading, LV_ALIGN_TOP_LEFT, 0, -2);
  lv_obj_t* subtitle = lv_label_create(button);
  lv_obj_set_width(subtitle, w - 18);
  lv_label_set_long_mode(subtitle, LV_LABEL_LONG_DOT);
  lv_label_set_text(subtitle, detail);
  lv_obj_set_style_text_font(subtitle, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_color(subtitle, lv_color_hex(primary ? 0x003526 : COLOR_MUTED), 0);
  lv_obj_align(subtitle, LV_ALIGN_BOTTOM_LEFT, 0, 2);
  return button;
}

static lv_obj_t* makeDashboardLabel(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* panel = makePanel(parent);
  lv_obj_set_size(panel, w, h);
  lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* label = lv_label_create(panel);
  lv_obj_set_width(label, w - 10);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
  return label;
}

static lv_obj_t* makePageTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* title = lv_label_create(parent);
  lv_label_set_text(title, text);
  lv_obj_set_width(title, SCREEN_W - 12);
  lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_text_font(title, UI_FONT_DEFAULT, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 2, 0);
  return title;
}

static lv_obj_t* makeNavButton(lv_obj_t* parent, const char* text, lv_align_t align, int x, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 92, 30);
  lv_obj_align(btn, align, x, 0);
  styleDarkObject(btn, 0x111A18);
  styleDarkBorder(btn, 0x315B50);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, 80);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static void buildNavBar(lv_obj_t* screen) {
  navBar = lv_obj_create(screen);
  lv_obj_set_size(navBar, SCREEN_W, NAV_BAR_H);
  lv_obj_align(navBar, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(navBar, 0x080D0B);
  lv_obj_set_style_border_color(navBar, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_side(navBar, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(navBar, 1, 0);
  lv_obj_set_style_pad_all(navBar, 4, 0);
  lv_obj_clear_flag(navBar, LV_OBJ_FLAG_SCROLLABLE);

  btnNavBack = makeNavButton(navBar, "Back", LV_ALIGN_LEFT_MID, 4, [](lv_event_t*) {
    if (currentPage == pageLauncher) { showPage(pageFunctions); return; }
    lv_obj_t* target = navigationDepth ? navigationStack[--navigationDepth] : pageLauncher;
    showPage(target, false);
  });
  btnNavHome = makeNavButton(navBar, "Home", LV_ALIGN_RIGHT_MID, -4, [](lv_event_t*) {
    navigationDepth = 0;
    showPage(pageLauncher, false);
  });
}

static lv_obj_t* makeReadonlyText(lv_obj_t* parent, int y, int h) {
  lv_obj_t* ta = lv_textarea_create(parent);
  lv_obj_set_size(ta, SCREEN_W - 16, h);
  lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, y);
  lv_textarea_set_cursor_click_pos(ta, false);
  lv_obj_clear_flag(ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  styleDarkTextArea(ta);
  return ta;
}

static bool setReadonlyTextStable(lv_obj_t* ta, const char* text, bool scrollTop) {
  if (!ta || !text) return false;
  const char* current = lv_textarea_get_text(ta);
  bool same = current && strcmp(current, text) == 0;
  lv_coord_t scrollY = scrollTop ? 0 : lv_obj_get_scroll_y(ta);
  if (!same) {
    lv_textarea_set_text(ta, text);
  }
  if (!same || scrollTop) {
    lv_textarea_set_cursor_pos(ta, 0);
    lv_obj_update_layout(ta);
    lv_obj_scroll_to_y(ta, scrollY, LV_ANIM_OFF);
  }
  return !same;
}

static void scrollReadonlyTextToBottom(lv_obj_t* ta) {
  if (!ta) return;
  const char* text = lv_textarea_get_text(ta);
  lv_textarea_set_cursor_pos(ta, text ? strlen(text) : 0);
  lv_obj_update_layout(ta);
  lv_obj_scroll_to_y(ta, 32767, LV_ANIM_OFF);
}

static bool setReadonlyTextNewest(lv_obj_t* ta, const char* text, bool forceScroll) {
  if (!ta || !text) return false;
  const char* current = lv_textarea_get_text(ta);
  bool same = current && strcmp(current, text) == 0;
  if (!same) {
    lv_textarea_set_text(ta, text);
  }
  if (!same || forceScroll) {
    scrollReadonlyTextToBottom(ta);
  }
  return !same;
}

static NodeRecord* findNode(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num) return &nodes[i];
  }
  return nullptr;
}

static uint32_t nodeAgeSeconds(const NodeRecord& node, uint32_t nowMs) {
  return node.lastHeardMs ? (nowMs - node.lastHeardMs) / 1000 : 0;
}

static double headingTo(double lat1, double lon1, double lat2, double lon2) {
  double dLon = (lon2 - lon1) * DEG_TO_RAD;
  double y = sin(dLon) * cos(lat2 * DEG_TO_RAD);
  double x = cos(lat1 * DEG_TO_RAD) * sin(lat2 * DEG_TO_RAD) -
             sin(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) * cos(dLon);
  double brng = atan2(y, x) * RAD_TO_DEG;
  return fmod(brng + 360.0, 360.0);
}

static const char* nodeSortLabel() {
  switch (nodeSortMode) {
    case NODE_SORT_SNR: return "snr";
    case NODE_SORT_DISTANCE: return "range";
    case NODE_SORT_NAME: return "name";
    default: return "heard";
  }
}

static const char* nodeFilterLabel() {
  switch (nodeFilterMode) {
    case NODE_FILTER_ACTIVE: return "active";
    case NODE_FILTER_POSITIONED: return "position";
    case NODE_FILTER_STALE: return "stale";
    case NODE_FILTER_NEARBY: return "nearby";
    default: return "all";
  }
}

static const char* nodeListViewLabel() {
  return nodeListViewMode == NODE_LIST_VIEW_COMPACT ? "compact" : "detail";
}

static void formatShortAge(uint32_t ageSeconds, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (ageSeconds < 60) snprintf(out, outSize, "%lus", (unsigned long)ageSeconds);
  else if (ageSeconds < 3600) snprintf(out, outSize, "%lum", (unsigned long)(ageSeconds / 60));
  else if (ageSeconds < 86400) snprintf(out, outSize, "%luh", (unsigned long)(ageSeconds / 3600));
  else snprintf(out, outSize, "%lud", (unsigned long)(ageSeconds / 86400));
}

static bool nodeHasRange(const NodeRecord& node) {
  return gpsStats.valid && node.hasPosition;
}

static double nodeRangeMeters(const NodeRecord& node) {
  if (!nodeHasRange(node)) return -1.0;
  return distanceMeters(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
}

static const char* nodeStatusLabel(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return "STALE";
  if (node.snr < -10.0f) return "WEAK";
  return "ACTIVE";
}

static uint32_t nodeRowBg(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return 0x0B100F;
  if (node.snr < -10.0f) return 0x1D1810;
  return 0x10201B;
}

static uint32_t nodeRowBorder(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return 0x2A3A35;
  if (node.snr < -10.0f) return 0x6F5A2E;
  return 0x2F705F;
}

static bool nodeMatchesFilter(const NodeRecord& node, uint32_t nowMs) {
  uint32_t age = nodeAgeSeconds(node, nowMs);
  switch (nodeFilterMode) {
    case NODE_FILTER_ACTIVE: return age <= 900;
    case NODE_FILTER_POSITIONED: return node.hasPosition;
    case NODE_FILTER_STALE: return age > 900;
    case NODE_FILTER_NEARBY: return nodeHasRange(node) && nodeRangeMeters(node) <= NODE_NEARBY_METERS;
    default: return true;
  }
}

static bool nodeComesBefore(const NodeRecord& a, const NodeRecord& b, uint32_t nowMs) {
  if (a.isFavorite != b.isFavorite) return a.isFavorite;
  switch (nodeSortMode) {
    case NODE_SORT_SNR:
      return a.snr > b.snr;
    case NODE_SORT_DISTANCE:
      if (gpsStats.valid) {
        bool aHasRange = a.hasPosition;
        bool bHasRange = b.hasPosition;
        if (aHasRange && bHasRange) {
          double da = distanceMeters(gpsStats.latitude, gpsStats.longitude, a.latitude, a.longitude);
          double db = distanceMeters(gpsStats.latitude, gpsStats.longitude, b.latitude, b.longitude);
          return da < db;
        }
        if (aHasRange != bHasRange) return aHasRange;
      }
      return nodeAgeSeconds(a, nowMs) < nodeAgeSeconds(b, nowMs);
    case NODE_SORT_NAME:
      return strcasecmp(a.name, b.name) < 0;
    default:
      return nodeAgeSeconds(a, nowMs) < nodeAgeSeconds(b, nowMs);
  }
}

static void formatRangeBearing(const NodeRecord& node, char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return;
  if (!gpsStats.valid || !node.hasPosition) {
    strlcpy(buffer, "range --", bufferSize);
    return;
  }
  double meters = distanceMeters(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
  double bearing = headingTo(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
  if (meters >= 1000.0) {
    snprintf(buffer, bufferSize, "%.1f km %.0f deg", meters / 1000.0, bearing);
  } else {
    snprintf(buffer, bufferSize, "%.0f m %.0f deg", meters, bearing);
  }
}

static void nodeListButtonEvent(lv_event_t* e) {
  uint32_t nodeNum = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
  showNodeDetail(nodeNum);
}

static void cycleNodeSort() {
  nodeSortMode = (NodeSortMode)((nodeSortMode + 1) % 4);
  refreshNodeList(true);
}

static void cycleNodeFilter() {
  nodeFilterMode = (NodeFilterMode)((nodeFilterMode + 1) % 5);
  refreshNodeList(true);
}

static void cycleNodeListView() {
  nodeListViewMode = nodeListViewMode == NODE_LIST_VIEW_COMPACT
                         ? NODE_LIST_VIEW_DETAILED
                         : NODE_LIST_VIEW_COMPACT;
  prefs.putBool("nodeCompact", nodeListViewMode == NODE_LIST_VIEW_COMPACT);
  refreshNodeList(true);
}

static void clearNodeList() {
  memset(nodes, 0, sizeof(nodes));
  nodeCount = 0;
  selectedNodeNum = 0;
  nodeSortMode = NODE_SORT_LAST_HEARD;
  nodeFilterMode = NODE_FILTER_ALL;
  lastNodeListRefreshMs = 0;
  nodeListClearedMs = millis();
  stats.onlineNodes = 0;
  stats.totalNodes = 0;
  mapRenderPending = true;
  appendLine(eventLog, LOG_SIZE, "[local] node list cleared\n");
  sendHeltecNodeDbReset();
  refreshNodeList(true);
  refreshDashboardLabels();
}

static void refreshNodeList(bool force) {
  if (!listNodes) return;
  uint32_t now = millis();
  if (!force && now - lastNodeListRefreshMs < 3000) return;
  lastNodeListRefreshMs = now;
  lv_obj_clean(listNodes);
  lv_obj_set_style_pad_all(listNodes, nodeListViewMode == NODE_LIST_VIEW_COMPACT ? 2 : 4, 0);
  lv_obj_set_style_pad_row(listNodes, nodeListViewMode == NODE_LIST_VIEW_COMPACT ? 2 : 6, 0);
  if (lblNodeListMode) {
    char modeText[48];
    snprintf(modeText, sizeof(modeText), "Sort: %s  Filter: %s  View: %s",
             nodeSortLabel(),
             nodeFilterLabel(),
             nodeListViewLabel());
    lv_label_set_text(lblNodeListMode, modeText);
  }
  size_t sorted[MAX_NODES];
  size_t sortedCount = 0;
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodeMatchesFilter(nodes[i], now)) sorted[sortedCount++] = i;
  }
  for (size_t i = 0; i < sortedCount; i++) {
    for (size_t j = i + 1; j < sortedCount; j++) {
      if (nodeComesBefore(nodes[sorted[j]], nodes[sorted[i]], now)) {
        size_t tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }
  if (sortedCount == 0) {
    const char* emptyText = nodeCount ? "No nodes match filter" : "No nodes heard yet";
    if (!nodeCount && nodeListClearedMs) emptyText = "Node list reset; waiting for fresh packets";
    lv_list_add_text(listNodes, emptyText);
    clearUiDirty(UI_DIRTY_NODES);
    return;
  }
  for (size_t listIndex = 0; listIndex < sortedCount; listIndex++) {
    NodeRecord& node = nodes[sorted[listIndex]];
    char title[48];
    snprintf(title, sizeof(title), "%.30s", node.name[0] ? node.name : nodeName(node.num));
    uint32_t ageSeconds = nodeAgeSeconds(node, now);
    bool compactView = nodeListViewMode == NODE_LIST_VIEW_COMPACT;

    if (compactView) {
      lv_obj_t* row = lv_obj_create(listNodes);
      lv_obj_set_width(row, lv_pct(100));
      lv_obj_set_height(row, 30);
      styleDarkObject(row, node.isFavorite ? 0x132A22 : 0x0B1210);
      styleDarkBorder(row, node.isFavorite ? COLOR_ACCENT : 0x203B35);
      lv_obj_set_style_border_width(row, node.isFavorite ? 1 : 0, LV_PART_MAIN);
      lv_obj_set_style_radius(row, node.isFavorite ? 4 : 2, 0);
      lv_obj_set_style_pad_all(row, 3, 0);
      lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(row, nodeListButtonEvent, LV_EVENT_CLICKED, (void*)(uintptr_t)node.num);

      lv_obj_t* favLabel = lv_label_create(row);
      lv_label_set_text(favLabel, node.isFavorite ? "F" : "");
      lv_obj_set_width(favLabel, 16);
      lv_label_set_long_mode(favLabel, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_color(favLabel, lv_color_hex(COLOR_ACCENT), 0);
      lv_obj_set_style_text_font(favLabel, UI_FONT_SMALL, 0);
      lv_obj_align(favLabel, LV_ALIGN_LEFT_MID, 0, 0);

      lv_obj_t* nameLabel = lv_label_create(row);
      lv_label_set_text(nameLabel, title);
      lv_obj_set_width(nameLabel, 116);
      lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_color(nameLabel, lv_color_hex(ageSeconds > 900 ? COLOR_MUTED : COLOR_TEXT), 0);
      lv_obj_set_style_text_font(nameLabel, UI_FONT_SMALL, 0);
      lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 18, 0);

      char ageText[8];
      char metaText[32];
      formatShortAge(ageSeconds, ageText, sizeof(ageText));
      snprintf(metaText, sizeof(metaText), "%s H%u %s",
               ageText,
               node.hopsAway,
               node.hasPosition ? "GPS" : "--");
      lv_obj_t* metaLabel = lv_label_create(row);
      lv_label_set_text(metaLabel, metaText);
      lv_obj_set_width(metaLabel, 82);
      lv_label_set_long_mode(metaLabel, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(metaLabel, LV_TEXT_ALIGN_RIGHT, 0);
      lv_obj_set_style_text_color(metaLabel, lv_color_hex(node.hasPosition ? COLOR_ACCENT : COLOR_MUTED), 0);
      lv_obj_set_style_text_font(metaLabel, UI_FONT_SMALL, 0);
      lv_obj_align(metaLabel, LV_ALIGN_RIGHT_MID, 0, 0);
      continue;
    }

    lv_obj_t* row = lv_obj_create(listNodes);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 60);
    styleDarkObject(row, nodeRowBg(node, now));
    styleDarkBorder(row, nodeRowBorder(node, now));
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, nodeListButtonEvent, LV_EVENT_CLICKED, (void*)(uintptr_t)node.num);

    lv_obj_t* nameLabel = lv_label_create(row);
    lv_label_set_text(nameLabel, title);
    lv_obj_set_width(nameLabel, 126);
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(nameLabel, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(nameLabel, UI_FONT_SMALL, 0);
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    char badgeText[24];
    lv_obj_t* badgeLabel = lv_label_create(row);
    snprintf(badgeText, sizeof(badgeText), "%s%s",
             node.isFavorite ? "FAV " : "",
             node.hasPosition ? "GPS" : "NO GPS");
    lv_label_set_text(badgeLabel, badgeText);
    lv_obj_set_width(badgeLabel, 78);
    lv_label_set_long_mode(badgeLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(badgeLabel, lv_color_hex(node.isFavorite ? COLOR_ACCENT : (node.hasPosition ? COLOR_ACCENT : COLOR_MUTED)), 0);
    lv_obj_set_style_text_align(badgeLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(badgeLabel, UI_FONT_SMALL, 0);
    lv_obj_align(badgeLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

    char rangeText[32];
    formatRangeBearing(node, rangeText, sizeof(rangeText));
    char detail[128];
    snprintf(detail, sizeof(detail), "!%08lX  H%u  %lus\n%.1f dB  %ld dBm  %s",
             (unsigned long)node.num,
             node.hopsAway,
             (unsigned long)ageSeconds,
             node.snr,
             (long)node.rssi,
             rangeText);
    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, detail);
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 16);
  }
  clearUiDirty(UI_DIRTY_NODES);
}

static void directSelectedNode() {
  if (!selectedNodeNum || !taDirectTo) return;
  char toText[12];
  snprintf(toText, sizeof(toText), "!%08lX", (unsigned long)selectedNodeNum);
  lv_textarea_set_text(taDirectTo, toText);
  showPage(pageDirectChat);
}

static void showSelectedNodeOnMap() {
  if (!selectedNodeNum) return;
  NodeRecord* selected = findNode(selectedNodeNum);
  mapNearbyMode = false;
  mapNodeDetailViewActive = selected != nullptr;
  mapCenterMode = selected ? MAP_CENTER_SELECTED_NODE : MAP_CENTER_CURRENT_DEVICE;
  mapFocusSelectedNode = selected && selected->hasPosition;
  mapView.follow = false;
  mapView.manual = false;
  ++mapView.revision;
  mapCanvasCached = false;
  mapRenderPending = true;
  markUiDirty(UI_DIRTY_MAP);
  lastMapUiRefreshMs = 0;
  showPage(pageGps);
}

static void showNodeDetail(uint32_t nodeNum) {
  if (selectedNodeNum != nodeNum) {
    nodeDetailLastText[0] = '\0';
  }
  selectedNodeNum = nodeNum;
  nodeDetailScrollTopPending = true;
  showPage(pageNodeDetail);
  refreshNodeDetailFavoriteButton();
}

static void refreshNodeDetailFavoriteButton() {
  if (!btnNodeFavorite || !lblNodeFavorite) return;
  NodeRecord* node = findNode(selectedNodeNum);
  bool fav = node && node->isFavorite;
  lv_label_set_text(lblNodeFavorite, fav ? "Unfav" : "Fav");
  styleDarkObject(btnNodeFavorite, fav ? 0x5A3D15 : COLOR_PANEL);
  styleDarkBorder(btnNodeFavorite, fav ? 0xC28C2C : 0x2F705F);
}

static void toggleSelectedNodeFavorite() {
  NodeRecord* node = findNode(selectedNodeNum);
  if (!node) return;
  bool newFavorite = !node->isFavorite;
  if (!sendHeltecFavoriteNode(node->num, newFavorite)) return;
  node->isFavorite = newFavorite;
  markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD);
  refreshNodeDetailFavoriteButton();
  refreshNodeList(true);
  refreshMessageDiagView();
}

static int8_t activeChatChannel() {
  if (activeChatInput == taFamilyInput) {
    return privateChannelIndex;
  }
  return PUBLIC_CHANNEL_INDEX;
}

static void sendFromInput(lv_obj_t* input, int8_t channelIndex) {
  if (!input) return;
  const char* msg = lv_textarea_get_text(input);
  if (msg && msg[0] && sendTextMessage(msg, channelIndex)) {
    lv_textarea_set_text(input, "");
  }
}

static void sendActiveFromScreen() {
  if (activeChatInput == taDirectInput) {
    sendDirectFromInput(taDirectTo, taDirectInput);
  } else if (activeChatInput != taDirectTo) {
    sendFromInput(activeChatInput, activeChatChannel());
  }
}

static void setUiLandscape(bool landscape) {
  if (!display) return;
  tft.setRotation(landscape ? 1 : 0);
  tft.invertDisplay(true);
  dispDrv.hor_res = landscape ? SCREEN_H : SCREEN_W;
  dispDrv.ver_res = landscape ? SCREEN_W : SCREEN_H;
  lv_disp_drv_update(display, &dispDrv);
  tft.fillScreen(TFT_BLACK);
}

static void closeLandscapeKeyboard(bool send) {
  if (!landscapeKeyboardOpen) return;
  if (keyboardText && activeChatInput) {
    lv_textarea_set_text(activeChatInput, lv_textarea_get_text(keyboardText));
  }
  if (send && landscapeKeyboardSendsMessage) {
    sendActiveFromScreen();
  }
  if (landscapeKeyboard) lv_keyboard_set_textarea(landscapeKeyboard, nullptr);
  landscapeKeyboardOpen = false;
  setUiLandscape(false);
  if (mainScreen) lv_scr_load(mainScreen);
}

static void landscapeKeyboardEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    closeLandscapeKeyboard(true);
  } else if (code == LV_EVENT_CANCEL) {
    closeLandscapeKeyboard(false);
  }
}

static void openLandscapeKeyboard(lv_obj_t* input, const char* prompt, size_t maxLength, bool sendsMessage) {
  if (!input || !keyboardScreen || !keyboardPrompt || !keyboardText || !landscapeKeyboard) return;
  activeChatInput = input;
  landscapeKeyboardSendsMessage = sendsMessage;
  lv_label_set_text(keyboardPrompt, prompt ? prompt : "Text");
  lv_textarea_set_text(keyboardText, lv_textarea_get_text(input));
  lv_textarea_set_max_length(keyboardText, maxLength);
  lv_keyboard_set_textarea(landscapeKeyboard, keyboardText);
  landscapeKeyboardOpen = true;
  lv_obj_clear_state(input, LV_STATE_FOCUSED);
  setUiLandscape(true);
  lv_scr_load(keyboardScreen);
  lv_obj_add_state(keyboardText, LV_STATE_FOCUSED);
}

static void inputEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_FOCUSED) {
    lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
    if (target == taDirectTo) openLandscapeKeyboard(target, "Direct recipient", 9, false);
    else if (target == taDirectInput) openLandscapeKeyboard(target, "Direct message", 233, true);
    else if (target == taFamilyInput) {
      char prompt[40];
      formatChannelPrompt(privateChannelLabel(), prompt, sizeof(prompt));
      openLandscapeKeyboard(target, prompt, 233, true);
    } else {
      char prompt[40];
      formatChannelPrompt(publicChannelLabel(), prompt, sizeof(prompt));
      openLandscapeKeyboard(target, prompt, 233, true);
    }
  } else if (code == LV_EVENT_DEFOCUSED) {
    if (!landscapeKeyboardOpen && keyboard) {
      lv_keyboard_set_textarea(keyboard, nullptr);
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  } else if (code == LV_EVENT_READY) {
    sendActiveFromScreen();
    if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  } else if (code == LV_EVENT_CANCEL) {
    if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  }
}

static void wifiInputEvent(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_FOCUSED) return;
  lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
  openLandscapeKeyboard(target, target == taWifiPass ? "WiFi password" : "WiFi SSID", target == taWifiPass ? 64 : 32, false);
}

static void deferWifiAction(uint8_t action) {
  deferredWifiAction = action;
  deferredWifiActionMs = millis();
}

static void ensureWifiLocalPage() {
  if (wifiLocalPageBuilt || !pageWifiLocal) return;
  wifiLocalPageBuilt = true;

  makePageTitle(pageWifiLocal, "Local Network");
  lv_obj_t* localWifiPanel = makePanel(pageWifiLocal);
  lv_obj_set_size(localWifiPanel, SCREEN_W - 12, 220);
  lv_obj_align(localWifiPanel, LV_ALIGN_TOP_MID, 0, 24);

  lv_obj_t* btnScanWifi = lv_btn_create(localWifiPanel);
  lv_obj_set_size(btnScanWifi, SCREEN_W - 40, 40);
  lv_obj_align(btnScanWifi, LV_ALIGN_TOP_MID, 0, 8);
  styleDarkObject(btnScanWifi, 0x2F705F, 0xFFFFFF);
  lv_obj_t* lblScanWifi = lv_label_create(btnScanWifi);
  lv_label_set_text(lblScanWifi, "Scan WiFi");
  styleBoundedLabel(lblScanWifi, SCREEN_W - 56, 0xFFFFFF);
  lv_obj_center(lblScanWifi);
  lv_obj_add_event_cb(btnScanWifi, [](lv_event_t*) {
    deferWifiAction(2);
  }, LV_EVENT_CLICKED, nullptr);

  taWifiSsid = lv_textarea_create(localWifiPanel);
  lv_obj_set_size(taWifiSsid, SCREEN_W - 40, 36);
  lv_obj_align(taWifiSsid, LV_ALIGN_TOP_MID, 0, 56);
  styleDarkTextArea(taWifiSsid);
  lv_textarea_set_one_line(taWifiSsid, true);
  lv_textarea_set_max_length(taWifiSsid, 32);
  lv_textarea_set_placeholder_text(taWifiSsid, "SSID");
  lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  lv_obj_add_event_cb(taWifiSsid, wifiInputEvent, LV_EVENT_FOCUSED, nullptr);

  taWifiPass = lv_textarea_create(localWifiPanel);
  lv_obj_set_size(taWifiPass, SCREEN_W - 40, 36);
  lv_obj_align(taWifiPass, LV_ALIGN_TOP_MID, 0, 100);
  styleDarkTextArea(taWifiPass);
  lv_textarea_set_one_line(taWifiPass, true);
  lv_textarea_set_password_mode(taWifiPass, true);
  lv_textarea_set_max_length(taWifiPass, 64);
  lv_textarea_set_placeholder_text(taWifiPass, "Password");
  lv_textarea_set_text(taWifiPass, wifiLocalPass);
  lv_obj_add_event_cb(taWifiPass, wifiInputEvent, LV_EVENT_FOCUSED, nullptr);

  lv_obj_t* btnSaveWifi = lv_btn_create(localWifiPanel);
  lv_obj_set_size(btnSaveWifi, SCREEN_W - 40, 40);
  lv_obj_align(btnSaveWifi, LV_ALIGN_TOP_MID, 0, 144);
  styleDarkObject(btnSaveWifi, COLOR_ACTION, 0xFFFFFF);
  lv_obj_t* lblSaveWifi = lv_label_create(btnSaveWifi);
  lv_label_set_text(lblSaveWifi, "Connect");
  styleBoundedLabel(lblSaveWifi, SCREEN_W - 56, 0x001B12);
  lv_obj_center(lblSaveWifi);
  lv_obj_add_event_cb(btnSaveWifi, [](lv_event_t*) {
    saveWifiCredentials();
  }, LV_EVENT_CLICKED, nullptr);
}

static void ensureWifiScanPage() {
  if (wifiScanPageBuilt || !pageWifiScan) return;
  wifiScanPageBuilt = true;

  makePageTitle(pageWifiScan, "Scan Networks");
  lblWifiScanStatus = lv_label_create(pageWifiScan);
  lv_label_set_text(lblWifiScanStatus, "Tap Refresh to scan");
  styleBoundedLabel(lblWifiScanStatus, SCREEN_W - 92, COLOR_MUTED);
  lv_obj_align(lblWifiScanStatus, LV_ALIGN_TOP_LEFT, 8, 24);
  lv_obj_t* btnRefreshScan = lv_btn_create(pageWifiScan);
  lv_obj_set_size(btnRefreshScan, 86, 30);
  lv_obj_align(btnRefreshScan, LV_ALIGN_TOP_RIGHT, -8, 20);
  styleDarkObject(btnRefreshScan, COLOR_ACTION, 0x001B12);
  lv_obj_t* lblRefreshScan = lv_label_create(btnRefreshScan);
  lv_label_set_text(lblRefreshScan, "Refresh");
  styleBoundedLabel(lblRefreshScan, 74, 0x001B12);
  lv_obj_center(lblRefreshScan);
  lv_obj_add_event_cb(btnRefreshScan, [](lv_event_t*) { requestWifiScan(); }, LV_EVENT_CLICKED, nullptr);
  listWifiScan = lv_list_create(pageWifiScan);
  lv_obj_set_size(listWifiScan, SCREEN_W - 12, 180);
  lv_obj_align(listWifiScan, LV_ALIGN_TOP_MID, 0, 58);
  styleDarkObject(listWifiScan, COLOR_PANEL);
}

static void processDeferredWifiAction() {
  if (!deferredWifiAction || millis() - deferredWifiActionMs < 50) return;
  uint8_t action = deferredWifiAction;
  deferredWifiAction = 0;
  if (action == 1) {
    ensureWifiLocalPage();
    showPage(pageWifiLocal);
  } else if (action == 2) {
    ensureWifiScanPage();
    showPage(pageWifiScan);
    requestWifiScan();
  }
}

static void buildLandscapeKeyboardScreen() {
  keyboardScreen = lv_obj_create(nullptr);
  styleDarkObject(keyboardScreen, COLOR_BG);
  lv_obj_set_style_border_width(keyboardScreen, 0, 0);
  lv_obj_clear_flag(keyboardScreen, LV_OBJ_FLAG_SCROLLABLE);

  keyboardPrompt = lv_label_create(keyboardScreen);
  lv_label_set_text(keyboardPrompt, "Message");
  styleBoundedLabel(keyboardPrompt, SCREEN_H - 16, COLOR_ACCENT);
  lv_obj_align(keyboardPrompt, LV_ALIGN_TOP_LEFT, 8, 5);

  keyboardText = lv_textarea_create(keyboardScreen);
  lv_obj_set_size(keyboardText, SCREEN_H - 16, 50);
  lv_obj_align(keyboardText, LV_ALIGN_TOP_MID, 0, 24);
  styleDarkTextArea(keyboardText);
  lv_textarea_set_one_line(keyboardText, true);
  lv_textarea_set_max_length(keyboardText, 233);
  lv_obj_add_event_cb(keyboardText, landscapeKeyboardEvent, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(keyboardText, landscapeKeyboardEvent, LV_EVENT_CANCEL, nullptr);

  landscapeKeyboard = lv_keyboard_create(keyboardScreen);
  lv_obj_set_size(landscapeKeyboard, SCREEN_H, 156);
  lv_obj_align(landscapeKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(landscapeKeyboard, COLOR_PANEL);
  lv_obj_set_style_bg_color(landscapeKeyboard, lv_color_hex(0x111A18), LV_PART_ITEMS);
  lv_obj_set_style_text_color(landscapeKeyboard, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_border_color(landscapeKeyboard, lv_color_hex(0x315B50), LV_PART_ITEMS);
  lv_obj_set_style_border_width(landscapeKeyboard, 1, LV_PART_ITEMS);
  lv_obj_add_event_cb(landscapeKeyboard, landscapeKeyboardEvent, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(landscapeKeyboard, landscapeKeyboardEvent, LV_EVENT_CANCEL, nullptr);
}

#include "HomeUi.inc"

static void buildScreenUi() {
  lv_obj_t* screen = lv_scr_act();
  mainScreen = screen;
  styleDarkObject(screen, COLOR_BG);
  lv_obj_set_style_text_font(screen, UI_FONT_DEFAULT, 0);

  buildStatusBar(screen);

  pageLauncher = makePage(screen);
  pageFunctions = makePage(screen);
  pageOperate = makePage(screen);
  pageDiagnose = makePage(screen);
  pageNodes = makePage(screen);
  pageNodeDetail = makePage(screen);
  pageMeshHealth = makePage(screen);
  pagePacketInspector = makePage(screen);
  pageTxHistory = makePage(screen);
  pagePublicChat = makePage(screen);
  pagePrivateChat = makePage(screen);
  pageDirectChat = makePage(screen);
  pageMessageDiag = makePage(screen);
  pageGps = makePage(screen);
  pageMapCoverage = makePage(screen);
  pageSystem = makePage(screen);
  pageSystemInterface = makePage(screen);
  pageSystemSerial = makePage(screen);
  pageSystemRadio = makePage(screen);
  pageSystemGps = makePage(screen);
  pageWifi = makePage(screen);
  pageWifiStats = makePage(screen);
  pageWifiLocal = makePage(screen);
  pageWifiScan = makePage(screen);
  pageBacklight = makePage(screen);
  pageSound = makePage(screen);
  pageBattery = makePage(screen);

  currentPage = pageLauncher;
  lv_obj_clear_flag(pageLauncher, LV_OBJ_FLAG_HIDDEN);

  buildHomeUi();

  makePageTitle(pageFunctions, "Menu");
  makeHomeAction(pageFunctions, "Messages", "Chat & direct", 6, 24, 106, 62, false,
                 [](lv_event_t*) { showPage(pageOperate); });
  makeHomeAction(pageFunctions, "Nodes", "Nearby radios", 118, 24, 106, 62, false, [](lv_event_t*) {
    refreshNodeList(true);
    showPage(pageNodes);
  });
  makeHomeAction(pageFunctions, "Map", "Location & GPS", 6, 94, 106, 62, false,
                 [](lv_event_t*) { showPage(pageGps); });
  makeHomeAction(pageFunctions, "TX Log", "Delivery & retry", 118, 94, 106, 62, false,
                 [](lv_event_t*) { showPage(pageTxHistory); });
  makeHomeAction(pageFunctions, "System", "Device settings", 6, 164, 106, 62, false,
                 [](lv_event_t*) { showPage(pageSystem); });

  makePageTitle(pageOperate, "Messages");
  makePageScrollable(pageOperate);
  lv_obj_t* publicTile = makeSystemTile(pageOperate, publicChannelLabel(), 0, 0, [](lv_event_t*) { showPage(pagePublicChat); });
  lblTilePublic = lv_obj_get_child(publicTile, 0);
  lv_obj_t* privateTile = makeSystemTile(pageOperate, privateChannelLabel(), 1, 0, [](lv_event_t*) { showPage(pagePrivateChat); });
  lblTilePrivate = lv_obj_get_child(privateTile, 0);
  makeSystemTile(pageOperate, "Direct", 0, 1, [](lv_event_t*) { showPage(pageDirectChat); });
  makeSystemTile(pageOperate, "TX Log", 1, 1, [](lv_event_t*) { showPage(pageTxHistory); });
  lblMsgPublic = makeDashboardLabel(pageOperate, 6, 120, 228, 34);
  lblMsgFamily = makeDashboardLabel(pageOperate, 6, 160, 228, 34);
  lblMsgDirect = makeDashboardLabel(pageOperate, 6, 200, 228, 34);

  makePageTitle(pageDiagnose, "Health");
  makePageScrollable(pageDiagnose);
  makeSystemTile(pageDiagnose, "Mesh", 0, 0, [](lv_event_t*) { showPage(pageMeshHealth); });
  makeSystemTile(pageDiagnose, "Packets", 1, 0, [](lv_event_t*) { showPage(pagePacketInspector); });
  makeSystemTile(pageDiagnose, "Serial", 0, 1, [](lv_event_t*) { showPage(pageSystemSerial); });
  makeSystemTile(pageDiagnose, "Radio", 1, 1, [](lv_event_t*) { showPage(pageSystemRadio); });
  makeSystemTile(pageDiagnose, "Interface", 0, 2, [](lv_event_t*) { showPage(pageSystemInterface); });
  makeSystemTile(pageDiagnose, "Power", 1, 2, [](lv_event_t*) { showPage(pageBattery); });
  makeSystemTile(pageDiagnose, "GPS", 0, 3, [](lv_event_t*) { showPage(pageSystemGps); });
  makeSystemTile(pageDiagnose, "WiFi", 1, 3, [](lv_event_t*) { showPage(pageWifiStats); });
  makeSystemTile(pageDiagnose, "Msg Test", 0, 4, [](lv_event_t*) {
    refreshMessageDiagView();
    showPage(pageMessageDiag);
  });

  makePageTitle(pageNodes, "Nodes");
  makeSmallButton(pageNodes, "Sort", 6, 21, 48, [](lv_event_t*) { cycleNodeSort(); });
  makeSmallButton(pageNodes, "Filter", 60, 21, 56, [](lv_event_t*) { cycleNodeFilter(); });
  makeSmallButton(pageNodes, "View", 122, 21, 46, [](lv_event_t*) { cycleNodeListView(); });
  makeSmallButton(pageNodes, "Clear", 174, 21, 60, [](lv_event_t*) { clearNodeList(); });
  lblNodeListMode = lv_label_create(pageNodes);
  lv_label_set_text(lblNodeListMode, "sort heard  filter all  view compact");
  lv_obj_set_width(lblNodeListMode, SCREEN_W - 12);
  lv_label_set_long_mode(lblNodeListMode, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(lblNodeListMode, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(lblNodeListMode, UI_FONT_SMALL, 0);
  lv_obj_align(lblNodeListMode, LV_ALIGN_TOP_LEFT, 6, 46);
  listNodes = lv_list_create(pageNodes);
  lv_obj_set_size(listNodes, SCREEN_W - 12, 188);
  lv_obj_align(listNodes, LV_ALIGN_TOP_MID, 0, 62);
  styleDarkObject(listNodes, COLOR_PANEL);
  styleDarkBorder(listNodes, 0x2F705F);
  lv_obj_set_style_pad_all(listNodes, 4, 0);
  lv_obj_set_style_pad_row(listNodes, 6, 0);

  makePageTitle(pageNodeDetail, "Node Detail");
  taNodeDetail = makeReadonlyText(pageNodeDetail, 22, 184);
  btnNodeFavorite = lv_btn_create(pageNodeDetail);
  lv_obj_set_size(btnNodeFavorite, 72, 22);
  lv_obj_align(btnNodeFavorite, LV_ALIGN_TOP_LEFT, 6, 214);
  styleDarkObject(btnNodeFavorite, COLOR_PANEL);
  styleDarkBorder(btnNodeFavorite, 0x2F705F);
  lv_obj_set_style_radius(btnNodeFavorite, 6, 0);
  lv_obj_set_style_shadow_width(btnNodeFavorite, 0, 0);
  lv_obj_add_flag(btnNodeFavorite, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btnNodeFavorite, [](lv_event_t*) { toggleSelectedNodeFavorite(); }, LV_EVENT_CLICKED, nullptr);
  lblNodeFavorite = lv_label_create(btnNodeFavorite);
  lv_label_set_text(lblNodeFavorite, "Fav");
  lv_obj_set_style_text_color(lblNodeFavorite, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(lblNodeFavorite);

  makeSmallButton(pageNodeDetail, "Msg", 84, 214, 72, [](lv_event_t*) { directSelectedNode(); });
  makeSmallButton(pageNodeDetail, "Map", 162, 214, 72, [](lv_event_t*) { showSelectedNodeOnMap(); });

  makePageTitle(pageMeshHealth, "Mesh Health");
  makePageScrollable(pageMeshHealth);
  lv_obj_t* meshHealthPanel = makePanel(pageMeshHealth);
  lv_obj_set_size(meshHealthPanel, SCREEN_W - 12, 392);
  lv_obj_align(meshHealthPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblMeshHealth = lv_label_create(meshHealthPanel);
  lv_label_set_text(lblMeshHealth, "Waiting for radio health...");
  styleBoundedLabel(lblMeshHealth, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pagePacketInspector, "Packets");
  taPacketInspector = makeReadonlyText(pagePacketInspector, 22, 226);
  lv_textarea_set_text(taPacketInspector, "No packets decoded yet");

  makePageTitle(pageTxHistory, "TX History");
  taTxHistory = makeReadonlyText(pageTxHistory, 22, 184);
  setReadonlyTextStable(taTxHistory, "No messages sent yet");
  btnTxRetry = makeActionButton(pageTxHistory, "Retry Last", 212, [](lv_event_t*) { retryLastTx(); });
  lv_obj_add_state(btnTxRetry, LV_STATE_DISABLED);

  lblPublicChatTitle = makePageTitle(pagePublicChat, "Primary Chat");
  lblPublicChannelStats = lv_label_create(pagePublicChat);
  lv_label_set_text(lblPublicChannelStats, "Ch 0: primary");
  styleBoundedLabel(lblPublicChannelStats, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPublicChannelStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taPublicChat = makeReadonlyText(pagePublicChat, 32, 154);
  lv_textarea_set_text(taPublicChat, "No public chat yet");
  taPublicInput = lv_textarea_create(pagePublicChat);
  lv_obj_set_size(taPublicInput, SCREEN_W - 76, 38);
  lv_obj_align(taPublicInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taPublicInput);
  lv_textarea_set_one_line(taPublicInput, true);
  lv_textarea_set_max_length(taPublicInput, 233);
  lv_textarea_set_placeholder_text(taPublicInput, "Primary message");
  lv_textarea_set_text(taPublicInput, "");
  lv_obj_add_event_cb(taPublicInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* publicSendBtn = lv_btn_create(pagePublicChat);
  lv_obj_set_size(publicSendBtn, 56, 38);
  lv_obj_align(publicSendBtn, LV_ALIGN_TOP_RIGHT, -6, 192);
  styleDarkObject(publicSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(publicSendBtn, 0, 0);
  lv_obj_add_event_cb(publicSendBtn, [](lv_event_t*) { sendFromInput(taPublicInput, PUBLIC_CHANNEL_INDEX); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(publicSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* publicSendLbl = lv_label_create(publicSendBtn);
  lv_label_set_text(publicSendLbl, "Send");
  lv_obj_set_style_text_color(publicSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(publicSendLbl);
  lv_obj_move_foreground(publicSendBtn);

  lblPrivateChatTitle = makePageTitle(pagePrivateChat, "Channel 1 Chat");
  lblPrivateChannelStats = lv_label_create(pagePrivateChat);
  lv_label_set_text(lblPrivateChannelStats, "Ch 1");
  styleBoundedLabel(lblPrivateChannelStats, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPrivateChannelStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taFamilyChat = makeReadonlyText(pagePrivateChat, 32, 154);
  lv_textarea_set_text(taFamilyChat, "No secondary chat yet");
  taFamilyInput = lv_textarea_create(pagePrivateChat);
  lv_obj_set_size(taFamilyInput, SCREEN_W - 76, 38);
  lv_obj_align(taFamilyInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taFamilyInput);
  lv_textarea_set_one_line(taFamilyInput, true);
  lv_textarea_set_max_length(taFamilyInput, 233);
  lv_textarea_set_placeholder_text(taFamilyInput, "Channel message");
  lv_textarea_set_text(taFamilyInput, "");
  lv_obj_add_event_cb(taFamilyInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* privateSendBtn = lv_btn_create(pagePrivateChat);
  lv_obj_set_size(privateSendBtn, 56, 38);
  lv_obj_align(privateSendBtn, LV_ALIGN_TOP_RIGHT, -6, 192);
  styleDarkObject(privateSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(privateSendBtn, 0, 0);
  lv_obj_add_event_cb(privateSendBtn, [](lv_event_t*) { sendFromInput(taFamilyInput, privateChannelIndex >= 0 ? privateChannelIndex : 1); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(privateSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* privateSendLbl = lv_label_create(privateSendBtn);
  lv_label_set_text(privateSendLbl, "Send");
  lv_obj_set_style_text_color(privateSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(privateSendLbl);
  lv_obj_move_foreground(privateSendBtn);

  lblPrivateChannelHint = lv_label_create(pagePrivateChat);
  lv_label_set_text(lblPrivateChannelHint, "Radio ch: not loaded");
  styleBoundedLabel(lblPrivateChannelHint, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPrivateChannelHint, LV_ALIGN_TOP_LEFT, 2, 236);

  makePageTitle(pageDirectChat, "Direct Messages");
  taDirectChat = makeReadonlyText(pageDirectChat, 22, 132);
  lv_textarea_set_text(taDirectChat, "No direct messages yet");
  taDirectTo = lv_textarea_create(pageDirectChat);
  lv_obj_set_size(taDirectTo, SCREEN_W - 12, 34);
  lv_obj_align(taDirectTo, LV_ALIGN_TOP_LEFT, 6, 158);
  styleDarkTextArea(taDirectTo);
  lv_textarea_set_one_line(taDirectTo, true);
  lv_textarea_set_max_length(taDirectTo, 9);
  lv_textarea_set_placeholder_text(taDirectTo, "To: !1234ABCD");
  lv_textarea_set_text(taDirectTo, "");
  lv_obj_add_event_cb(taDirectTo, inputEvent, LV_EVENT_ALL, nullptr);
  taDirectInput = lv_textarea_create(pageDirectChat);
  lv_obj_set_size(taDirectInput, SCREEN_W - 76, 38);
  lv_obj_align(taDirectInput, LV_ALIGN_TOP_LEFT, 6, 198);
  styleDarkTextArea(taDirectInput);
  lv_textarea_set_one_line(taDirectInput, true);
  lv_textarea_set_max_length(taDirectInput, 233);
  lv_textarea_set_placeholder_text(taDirectInput, "Direct message");
  lv_textarea_set_text(taDirectInput, "");
  lv_obj_add_event_cb(taDirectInput, inputEvent, LV_EVENT_ALL, nullptr);
  lv_obj_t* directSendBtn = lv_btn_create(pageDirectChat);
  lv_obj_set_size(directSendBtn, 56, 38);
  lv_obj_align(directSendBtn, LV_ALIGN_TOP_RIGHT, -6, 198);
  styleDarkObject(directSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(directSendBtn, 0, 0);
  lv_obj_add_event_cb(directSendBtn, [](lv_event_t*) { sendDirectFromInput(taDirectTo, taDirectInput); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(directSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* directSendLbl = lv_label_create(directSendBtn);
  lv_label_set_text(directSendLbl, "Send");
  lv_obj_set_style_text_color(directSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(directSendLbl);
  lv_obj_move_foreground(directSendBtn);

  makePageTitle(pageMessageDiag, "Message Test");
  makePageScrollable(pageMessageDiag);
  lv_obj_t* msgDiagPanel = makePanel(pageMessageDiag);
  lv_obj_set_size(msgDiagPanel, SCREEN_W - 12, 420);
  lv_obj_align(msgDiagPanel, LV_ALIGN_TOP_MID, 0, 24);

  lblMsgDiagStatus = lv_label_create(msgDiagPanel);
  lv_label_set_text(lblMsgDiagStatus, msgDiagStatus);
  styleBoundedLabel(lblMsgDiagStatus, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMsgDiagStatus, LV_ALIGN_TOP_LEFT, 2, 4);

  lv_obj_t* lblMsgDiagLabelA = lv_label_create(msgDiagPanel);
  lv_label_set_text(lblMsgDiagLabelA, "Target A");
  styleBoundedLabel(lblMsgDiagLabelA, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMsgDiagLabelA, LV_ALIGN_TOP_LEFT, 2, 42);
  dropdownMsgDiagA = lv_dropdown_create(msgDiagPanel);
  lv_dropdown_set_options(dropdownMsgDiagA, "Loading...");
  lv_obj_set_size(dropdownMsgDiagA, SCREEN_W - 36, 34);
  lv_obj_align(dropdownMsgDiagA, LV_ALIGN_TOP_MID, 0, 62);
  styleDarkObject(dropdownMsgDiagA, COLOR_INPUT, COLOR_TEXT);
  styleDarkBorder(dropdownMsgDiagA, 0x2F705F);
  lv_obj_set_style_radius(dropdownMsgDiagA, 6, 0);
  lv_obj_add_event_cb(dropdownMsgDiagA, messageDiagDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* lblMsgDiagLabelB = lv_label_create(msgDiagPanel);
  lv_label_set_text(lblMsgDiagLabelB, "Target B");
  styleBoundedLabel(lblMsgDiagLabelB, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMsgDiagLabelB, LV_ALIGN_TOP_LEFT, 2, 104);
  dropdownMsgDiagB = lv_dropdown_create(msgDiagPanel);
  lv_dropdown_set_options(dropdownMsgDiagB, "Loading...");
  lv_obj_set_size(dropdownMsgDiagB, SCREEN_W - 36, 34);
  lv_obj_align(dropdownMsgDiagB, LV_ALIGN_TOP_MID, 0, 124);
  styleDarkObject(dropdownMsgDiagB, COLOR_INPUT, COLOR_TEXT);
  styleDarkBorder(dropdownMsgDiagB, 0x2F705F);
  lv_obj_set_style_radius(dropdownMsgDiagB, 6, 0);
  lv_obj_add_event_cb(dropdownMsgDiagB, messageDiagDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* lblMsgDiagMessage = lv_label_create(msgDiagPanel);
  lv_label_set_text(lblMsgDiagMessage, "Message");
  styleBoundedLabel(lblMsgDiagMessage, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMsgDiagMessage, LV_ALIGN_TOP_LEFT, 2, 166);
  taMsgDiagMessage = lv_textarea_create(msgDiagPanel);
  lv_obj_set_size(taMsgDiagMessage, SCREEN_W - 36, 56);
  lv_obj_align(taMsgDiagMessage, LV_ALIGN_TOP_MID, 0, 186);
  styleDarkTextArea(taMsgDiagMessage);
  lv_textarea_set_one_line(taMsgDiagMessage, true);
  lv_textarea_set_max_length(taMsgDiagMessage, 233);
  lv_textarea_set_placeholder_text(taMsgDiagMessage, "Test message");
  lv_textarea_set_text(taMsgDiagMessage, "");
  lv_obj_add_event_cb(taMsgDiagMessage, inputEvent, LV_EVENT_ALL, nullptr);

  makeSmallButton(msgDiagPanel, "Send A", 6, 252, 68, messageDiagSendAEvent);
  makeSmallButton(msgDiagPanel, "Send B", 80, 252, 68, messageDiagSendBEvent);
  makeSmallButton(msgDiagPanel, "Swap", 154, 252, 68, messageDiagSwapEvent);

  lblMsgDiagDetail = lv_label_create(msgDiagPanel);
  lv_label_set_text(lblMsgDiagDetail, msgDiagDetail);
  styleBoundedLabel(lblMsgDiagDetail, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMsgDiagDetail, LV_ALIGN_TOP_LEFT, 2, 292);

  // Fixed layout: dragging belongs to the map, never to a scrolling page.
  lv_obj_clear_flag(pageGps, LV_OBJ_FLAG_SCROLLABLE);
  auto mapButton = [](const char* text, int x, int y, int width, lv_event_cb_t callback) {
    lv_obj_t* button = lv_btn_create(pageGps);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, 44);
    styleDarkObject(button, COLOR_PANEL);
    styleDarkBorder(button, COLOR_BORDER);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_pad_all(button, 2, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(COLOR_ACCENT), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(button, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label = lv_label_create(button);
    styleBoundedLabel(label, width - 4, COLOR_TEXT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return button;
  };
  btnMapZoomOut = mapButton("-", 0, 0, 64, [](lv_event_t*) { setMapZoom(mapView.zoom - 1); });
  btnMapZoomIn = mapButton("+", 166, 0, 64, [](lv_event_t*) { setMapZoom(mapView.zoom + 1); });
  btnMapCoverage = mapButton("Zoom 14\nCoverage", 68, 0, 94, [](lv_event_t*) {
    showPage(pageMapCoverage);
    lastMapUiRefreshMs = 0;
    refreshMapUi();
  });
  lblMapZoom = lv_obj_get_child(btnMapCoverage, 0);
  lv_label_set_long_mode(lblMapZoom, LV_LABEL_LONG_WRAP);
  btnMapLocate = mapButton("My location", 0, 188, 111, [](lv_event_t*) { setMapFollow(true); });
  btnMapFollow = mapButton("Follow on", 119, 188, 111, [](lv_event_t*) { setMapFollow(!mapView.follow); });
  lblMapFollow = lv_obj_get_child(btnMapFollow, 0);

  mapPlot = makePanel(pageGps);
  lv_obj_set_size(mapPlot, MAP_PLOT_W, activeMapPlotHeight());
  lv_obj_align(mapPlot, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_set_style_bg_color(mapPlot, lv_color_hex(COLOR_INPUT), 0);
  lv_obj_set_style_pad_all(mapPlot, 0, 0);
  lv_obj_clear_flag(mapPlot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(mapPlot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_add_event_cb(mapPlot, mapDragEvent, LV_EVENT_ALL, nullptr);
  mapLayer = lv_obj_create(mapPlot);
  lv_obj_remove_style_all(mapLayer);
  lv_obj_set_pos(mapLayer, 0, 0);
  lv_obj_set_size(mapLayer, MAP_PLOT_W, MAP_PLOT_H);
  lv_obj_clear_flag(mapLayer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  if (mapCanvasBuf) {
    mapCanvas = lv_canvas_create(mapLayer);
    lv_canvas_set_buffer(mapCanvas, mapCanvasBuf, MAP_PLOT_W, MAP_PLOT_H, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
    lv_obj_set_pos(mapCanvas, 0, 0);
    lv_obj_clear_flag(mapCanvas, LV_OBJ_FLAG_CLICKABLE);
  }
  for (size_t i = 0; i < MAP_DOT_COUNT; i++) {
    mapDots[i] = lv_obj_create(mapLayer);
    styleDarkObject(mapDots[i], COLOR_ACCENT);
    lv_obj_set_size(mapDots[i], 8, 8);
    lv_obj_set_style_radius(mapDots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(mapDots[i], 0, 0);
    lv_obj_set_style_bg_color(mapDots[i], lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_add_flag(mapDots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mapDots[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(mapDots[i], LV_OBJ_FLAG_CLICKABLE);
  }
  mapScale = lv_obj_create(mapPlot);
  lv_obj_set_size(mapScale, 90, 29);
  lv_obj_align(mapScale, LV_ALIGN_BOTTOM_LEFT, 4, -4);
  styleDarkObject(mapScale, COLOR_BG);
  lv_obj_set_style_bg_opa(mapScale, LV_OPA_80, 0);
  lv_obj_set_style_border_width(mapScale, 0, 0);
  lv_obj_set_style_pad_all(mapScale, 0, 0);
  lv_obj_set_style_radius(mapScale, 3, 0);
  lv_obj_clear_flag(mapScale, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lblMapScale = lv_label_create(mapScale);
  styleBoundedLabel(lblMapScale, 82, COLOR_TEXT);
  lv_obj_set_pos(lblMapScale, 4, 0);
  mapScaleBar = lv_obj_create(mapScale);
  lv_obj_remove_style_all(mapScaleBar);
  lv_obj_set_pos(mapScaleBar, 4, 23);
  lv_obj_set_size(mapScaleBar, 60, 2);
  styleDarkObject(mapScaleBar, COLOR_TEXT);
  mapScaleEnd = lv_obj_create(mapScale);
  lv_obj_remove_style_all(mapScaleEnd);
  lv_obj_set_pos(mapScaleEnd, 63, 19);
  lv_obj_set_size(mapScaleEnd, 1, 6);
  styleDarkObject(mapScaleEnd, COLOR_TEXT);
  lv_obj_t* scaleStart = lv_obj_create(mapScale);
  lv_obj_remove_style_all(scaleStart);
  lv_obj_set_pos(scaleStart, 4, 19);
  lv_obj_set_size(scaleStart, 1, 6);
  styleDarkObject(scaleStart, COLOR_TEXT);
  for (auto* child : {lblMapScale, mapScaleBar, mapScaleEnd, scaleStart}) lv_obj_clear_flag(child, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(mapScale, LV_OBJ_FLAG_HIDDEN);
  lblMapNotice = lv_label_create(mapPlot);
  styleBoundedLabel(lblMapNotice, MAP_PLOT_W - 16, COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(lblMapNotice, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_bg_color(lblMapNotice, lv_color_hex(COLOR_BG), 0);
  lv_obj_set_style_bg_opa(lblMapNotice, LV_OPA_80, 0);
  lv_label_set_text(lblMapNotice, "Waiting for location");
  lv_obj_center(lblMapNotice);
  lv_obj_clear_flag(lblMapNotice, LV_OBJ_FLAG_CLICKABLE);
  lblMapCoverage = lv_label_create(mapPlot);
  styleBoundedLabel(lblMapCoverage, MAP_PLOT_W - 8, COLOR_TEXT);
  lv_obj_set_pos(lblMapCoverage, 3, 2);
  lv_obj_set_style_bg_color(lblMapCoverage, lv_color_hex(COLOR_BG), 0);
  lv_obj_set_style_bg_opa(lblMapCoverage, LV_OPA_80, 0);
  lv_obj_clear_flag(lblMapCoverage, LV_OBJ_FLAG_CLICKABLE);
  lblMapStats = lv_label_create(pageGps);
  styleBoundedLabel(lblMapStats, 230, COLOR_MUTED);
  lv_label_set_text(lblMapStats, "Waiting for location");
  lv_obj_set_pos(lblMapStats, 0, 236);
  updateMapControls();

  makePageTitle(pageMapCoverage, "Map coverage");
  lblMapCoverageSummary = lv_label_create(pageMapCoverage);
  styleBoundedLabel(lblMapCoverageSummary, 230, COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(lblMapCoverageSummary, 0, 24);
  for (int i = 0; i < MapCoverage::levels; ++i) {
    lv_obj_t* row = makePanel(pageMapCoverage);
    lv_obj_set_pos(row, 0, 62 + i * 34);
    lv_obj_set_size(row, 230, 32);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lblMapCoverageRows[i] = lv_label_create(row);
    styleBoundedLabel(lblMapCoverageRows[i], 210, COLOR_TEXT);
    lv_obj_center(lblMapCoverageRows[i]);
  }
  lv_obj_t* coverageHint = lv_label_create(pageMapCoverage);
  styleBoundedLabel(coverageHint, 230, COLOR_MUTED);
  lv_label_set_text(coverageHint, "Availability at this map center");
  lv_obj_set_pos(coverageHint, 0, 234);
  updateMapCoverageUi();

  makePageTitle(pageSystem, "System");
  makePageScrollable(pageSystem);
  makeSystemTile(pageSystem, "Health", 0, 0, [](lv_event_t*) { showPage(pageDiagnose); });
  makeSystemTile(pageSystem, "Interface", 1, 0, [](lv_event_t*) { showPage(pageSystemInterface); });
  makeSystemTile(pageSystem, "Radio", 0, 1, [](lv_event_t*) { showPage(pageSystemRadio); });
  makeSystemTile(pageSystem, "Serial", 1, 1, [](lv_event_t*) { showPage(pageSystemSerial); });
  makeSystemTile(pageSystem, "Battery", 0, 2, [](lv_event_t*) { showPage(pageBattery); });
  makeSystemTile(pageSystem, "GPS", 1, 2, [](lv_event_t*) { showPage(pageSystemGps); });
  makeSystemTile(pageSystem, "WiFi", 0, 3, [](lv_event_t*) { showPage(pageWifi); });
  makeSystemTile(pageSystem, "Display", 1, 3, [](lv_event_t*) { showPage(pageBacklight); });
  makeSystemTile(pageSystem, "Sound", 0, 4, [](lv_event_t*) { showPage(pageSound); });
  refreshMessageDiagView();

  makePageTitle(pageSystemInterface, "Interface");
  makePageScrollable(pageSystemInterface);
  lv_obj_t* interfacePanel = makePanel(pageSystemInterface);
  lv_obj_set_size(interfacePanel, SCREEN_W - 12, 330);
  lv_obj_align(interfacePanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemInterface = lv_label_create(interfacePanel);
  lv_label_set_text(lblSystemInterface, "Interface ready");
  styleBoundedLabel(lblSystemInterface, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemSerial, "Serial Link");
  makePageScrollable(pageSystemSerial);
  lv_obj_t* serialPanel = makePanel(pageSystemSerial);
  lv_obj_set_size(serialPanel, SCREEN_W - 12, 300);
  lv_obj_align(serialPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemSerial = lv_label_create(serialPanel);
  lv_label_set_text(lblSystemSerial, "Serial ready");
  styleBoundedLabel(lblSystemSerial, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemRadio, "Radio Stats");
  makePageScrollable(pageSystemRadio);
  lv_obj_t* radioPanel = makePanel(pageSystemRadio);
  lv_obj_set_size(radioPanel, SCREEN_W - 12, 300);
  lv_obj_align(radioPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemRadio = lv_label_create(radioPanel);
  lv_label_set_text(lblSystemRadio, "Radio ready");
  styleBoundedLabel(lblSystemRadio, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemGps, "GPS Stats");
  makePageScrollable(pageSystemGps);
  lv_obj_t* gpsPanel = makePanel(pageSystemGps);
  lv_obj_set_size(gpsPanel, SCREEN_W - 12, 430);
  lv_obj_align(gpsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblGpsStats = lv_label_create(gpsPanel);
  lv_label_set_text(lblGpsStats, "Waiting for CYD GPS UART...");
  styleBoundedLabel(lblGpsStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageWifi, "WiFi");
  makePageScrollable(pageWifi);
  lv_obj_t* wifiPanel = makePanel(pageWifi);
  lv_obj_set_size(wifiPanel, UI_PANEL_W, 142);
  lv_obj_align(wifiPanel, LV_ALIGN_TOP_MID, 0, 26);
  lv_obj_t* wifiToggleLabel = lv_label_create(wifiPanel);
  lv_label_set_text(wifiToggleLabel, "WiFi On/Off");
  styleBoundedLabel(wifiToggleLabel, 118, COLOR_TEXT);
  lv_obj_align(wifiToggleLabel, LV_ALIGN_TOP_LEFT, 2, 4);
  swWifiEnabled = lv_switch_create(wifiPanel);
  lv_obj_align(swWifiEnabled, LV_ALIGN_TOP_RIGHT, -2, 0);
  if (wifiEnabled) lv_obj_add_state(swWifiEnabled, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swWifiEnabled, [](lv_event_t* e) {
    setWifiEnabled(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* wifiModeLabel = lv_label_create(wifiPanel);
  lv_label_set_text(wifiModeLabel, "Local / AP");
  styleBoundedLabel(wifiModeLabel, 118, COLOR_TEXT);
  lv_obj_align(wifiModeLabel, LV_ALIGN_TOP_LEFT, 2, 42);
  swWifiApMode = lv_switch_create(wifiPanel);
  lv_obj_align(swWifiApMode, LV_ALIGN_TOP_RIGHT, -2, 38);
  if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swWifiApMode, [](lv_event_t* e) {
    setWifiApMode(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lblWifiState = lv_label_create(wifiPanel);
  lv_label_set_text(lblWifiState, "WiFi starting...");
  styleBoundedLabel(lblWifiState, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblWifiState, LV_ALIGN_TOP_LEFT, 2, 80);
  makeActionButton(pageWifi, "Local Network", 170, [](lv_event_t*) {
    deferWifiAction(1);
  });
  makeActionButton(pageWifi, "WiFi Stats", 214, [](lv_event_t*) { showPage(pageWifiStats); });

  makePageTitle(pageWifiStats, "WiFi Stats");
  makePageScrollable(pageWifiStats);
  lv_obj_t* wifiStatsPanel = makePanel(pageWifiStats);
  lv_obj_set_size(wifiStatsPanel, SCREEN_W - 12, 330);
  lv_obj_align(wifiStatsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblWifiStats = lv_label_create(wifiStatsPanel);
  lv_label_set_text(lblWifiStats, "WiFi stats unavailable");
  styleBoundedLabel(lblWifiStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageBacklight, "Display");
  makePageScrollable(pageBacklight);
  lv_obj_t* backlightPanel = makePanel(pageBacklight);
  lv_obj_set_size(backlightPanel, SCREEN_W - 12, 206);
  lv_obj_align(backlightPanel, LV_ALIGN_TOP_MID, 0, 30);
  lblBacklight = lv_label_create(backlightPanel);
  lv_label_set_text(lblBacklight, "Brightness: 10%");
  styleBoundedLabel(lblBacklight, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblBacklight, LV_ALIGN_TOP_LEFT, 2, 4);
  sliderBacklight = lv_slider_create(backlightPanel);
  lv_obj_set_size(sliderBacklight, SCREEN_W - 42, 24);
  lv_obj_align(sliderBacklight, LV_ALIGN_TOP_MID, 0, 54);
  lv_slider_set_range(sliderBacklight, 10, 100);
  lv_slider_set_value(sliderBacklight, backlightPercent, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(0x16342C), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(COLOR_ACTION), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(COLOR_TEXT), LV_PART_KNOB);
  lv_obj_add_event_cb(sliderBacklight, backlightSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t* readableLabel = lv_label_create(backlightPanel);
  lv_label_set_text(readableLabel, "Readable Text");
  styleBoundedLabel(readableLabel, 118, COLOR_TEXT);
  lv_obj_align(readableLabel, LV_ALIGN_TOP_LEFT, 2, 92);
  swReadableText = lv_switch_create(backlightPanel);
  lv_obj_align(swReadableText, LV_ALIGN_TOP_RIGHT, -2, 86);
  if (readableTextMode) lv_obj_add_state(swReadableText, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swReadableText, [](lv_event_t* e) {
    setReadableTextMode(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED), true);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lblTextMode = lv_label_create(backlightPanel);
  lv_label_set_text(lblTextMode, readableTextMode ? "Readable font" : "Compact font");
  styleBoundedLabel(lblTextMode, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblTextMode, LV_ALIGN_TOP_LEFT, 2, 126);
  lv_obj_t* backlightHint = lv_label_create(backlightPanel);
  lv_label_set_text(backlightHint, "Dimmer saves battery.");
  styleBoundedLabel(backlightHint, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(backlightHint, LV_ALIGN_TOP_LEFT, 2, 164);

  makePageTitle(pageSound, "Sound");
  makePageScrollable(pageSound);
  lv_obj_t* soundPanel = makePanel(pageSound);
  lv_obj_set_size(soundPanel, SCREEN_W - 12, 292);
  lv_obj_align(soundPanel, LV_ALIGN_TOP_MID, 0, 30);
  lblToneVolume = lv_label_create(soundPanel);
  lv_label_set_text(lblToneVolume, "Volume: 50%  Starting audio\nMsg: Bell  DM: Bell");
  styleBoundedLabel(lblToneVolume, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblToneVolume, LV_ALIGN_TOP_LEFT, 2, 4);
  sliderToneVolume = lv_slider_create(soundPanel);
  lv_obj_set_size(sliderToneVolume, SCREEN_W - 42, 24);
  lv_obj_align(sliderToneVolume, LV_ALIGN_TOP_MID, 0, 74);
  lv_slider_set_range(sliderToneVolume, 0, 100);
  lv_slider_set_value(sliderToneVolume, toneVolumePercent, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(sliderToneVolume, lv_color_hex(0x16342C), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sliderToneVolume, lv_color_hex(COLOR_ACTION), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sliderToneVolume, lv_color_hex(COLOR_TEXT), LV_PART_KNOB);
  lv_obj_add_event_cb(sliderToneVolume, toneVolumeSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t* lblMessageTone = lv_label_create(soundPanel);
  lv_label_set_text(lblMessageTone, "Message tone");
  styleBoundedLabel(lblMessageTone, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMessageTone, LV_ALIGN_TOP_LEFT, 2, 110);
  dropdownToneMessage = lv_dropdown_create(soundPanel);
  setToneDropdownOptions(dropdownToneMessage);
  lv_dropdown_set_selected(dropdownToneMessage, (uint16_t)toneMessageStyle);
  lv_obj_set_size(dropdownToneMessage, SCREEN_W - 42, 34);
  lv_obj_align(dropdownToneMessage, LV_ALIGN_TOP_MID, 0, 130);
  styleDarkObject(dropdownToneMessage, COLOR_INPUT, COLOR_TEXT);
  styleDarkBorder(dropdownToneMessage, 0x2F705F);
  lv_obj_set_style_radius(dropdownToneMessage, 6, 0);
  lv_obj_add_event_cb(dropdownToneMessage, toneMessageDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t* lblDirectTone = lv_label_create(soundPanel);
  lv_label_set_text(lblDirectTone, "Direct tone");
  styleBoundedLabel(lblDirectTone, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblDirectTone, LV_ALIGN_TOP_LEFT, 2, 174);
  dropdownToneDirect = lv_dropdown_create(soundPanel);
  setToneDropdownOptions(dropdownToneDirect);
  lv_dropdown_set_selected(dropdownToneDirect, (uint16_t)toneDirectStyle);
  lv_obj_set_size(dropdownToneDirect, SCREEN_W - 42, 34);
  lv_obj_align(dropdownToneDirect, LV_ALIGN_TOP_MID, 0, 194);
  styleDarkObject(dropdownToneDirect, COLOR_INPUT, COLOR_TEXT);
  styleDarkBorder(dropdownToneDirect, 0x2F705F);
  lv_obj_set_style_radius(dropdownToneDirect, 6, 0);
  lv_obj_add_event_cb(dropdownToneDirect, toneDirectDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  makeSmallButton(soundPanel, "Test Msg", 8, 248, 106, [](lv_event_t*) {
    queueToneStyle(toneMessageStyle);
  });
  makeSmallButton(soundPanel, "Test DM", 126, 248, 106, [](lv_event_t*) {
    queueToneStyle(toneDirectStyle);
  });
  updateToneVolumeLabel();

  makePageTitle(pageBattery, "Battery");
  makePageScrollable(pageBattery);
  lv_obj_t* batteryPanel = makePanel(pageBattery);
  lv_obj_set_size(batteryPanel, SCREEN_W - 12, 470);
  lv_obj_align(batteryPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblBatteryStats = lv_label_create(batteryPanel);
  lv_label_set_text(lblBatteryStats, "Reading S3 battery...");
  styleBoundedLabel(lblBatteryStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  buildNavBar(screen);
  showPage(pageLauncher, false);

  keyboard = lv_keyboard_create(screen);
  lv_obj_set_size(keyboard, SCREEN_W, 132);
  lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(keyboard, COLOR_PANEL);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x111A18), LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_border_color(keyboard, lv_color_hex(0x315B50), LV_PART_ITEMS);
  lv_obj_set_style_border_width(keyboard, 1, LV_PART_ITEMS);
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  refreshChannelUiLabels();
}

static void rebuildScreenUi(lv_obj_t* targetPage) {
  if (!display) return;
  lv_obj_t* screen = lv_scr_act();
  if (!screen) return;
  if (keyboardScreen) {
    lv_obj_del(keyboardScreen);
    keyboardScreen = nullptr;
    keyboardPrompt = nullptr;
    keyboardText = nullptr;
    landscapeKeyboard = nullptr;
    landscapeKeyboardOpen = false;
  }
  lv_obj_clean(screen);
  clearUiObjectPointers();
  buildScreenUi();
  buildLandscapeKeyboardScreen();
  if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  if (taWifiPass) lv_textarea_set_text(taWifiPass, wifiLocalPass);
  if (swWifiApMode) {
    if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiApMode, LV_STATE_CHECKED);
  }
  if (targetPage) showPage(targetPage, false);
  refreshDashboardLabels();
  refreshScreenUi();
}

static void setReadableTextMode(bool enabled, bool save) {
  if (readableTextMode == enabled && !save) return;
  readableTextMode = enabled;
  applyFontMode();
  if (save) prefs.putBool("readableText", readableTextMode);
  if (lblTextMode) {
    lv_label_set_text(lblTextMode, readableTextMode ? "Readable font" : "Compact font");
  }
  if (swReadableText) {
    if (readableTextMode) lv_obj_add_state(swReadableText, LV_STATE_CHECKED);
    else lv_obj_clear_state(swReadableText, LV_STATE_CHECKED);
  }
  if (mainScreen) {
    rebuildScreenUi(nullptr);
    showPage(pageBacklight, false);
  }
}

static void initScreen() {
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);

  initBacklight();

  lv_init();
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, SCREEN_W * 24);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_W;
  dispDrv.ver_res = SCREEN_H;
  dispDrv.flush_cb = lvFlush;
  dispDrv.draw_buf = &drawBuf;
  display = lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvTouchRead;
  lv_indev_drv_register(&indevDrv);

  buildScreenUi();
  buildLandscapeKeyboardScreen();
}

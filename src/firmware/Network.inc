// Internal implementation module; included once by the main sketch.
static void appendLine(char* buffer, size_t bufferSize, const char* line) {
  if (!buffer || !line || bufferSize == 0) return;
  size_t used = strlen(buffer);
  size_t incoming = strlen(line);
  if (used + incoming + 1 >= bufferSize) {
    const char* marker = "--- log trimmed ---\n";
    size_t markerLen = strlen(marker);
    size_t keep = bufferSize / 2;
    memmove(buffer, buffer + used - keep, keep);
    memmove(buffer + markerLen, buffer, keep);
    memcpy(buffer, marker, markerLen);
    buffer[markerLen + keep] = '\0';
    used = strlen(buffer);
  }
  strncat(buffer, line, bufferSize - used - 1);

  if (buffer == eventLog) {
    appendSdLine(SD_EVENTS_PATH, line);
    markUiDirty(UI_DIRTY_EVENT_LOG | UI_DIRTY_DASHBOARD);
  } else if (buffer == packetLog) {
    markUiDirty(UI_DIRTY_PACKET_LOG | UI_DIRTY_SYSTEM);
  } else if (buffer == publicChatLog) {
    appendSdLine(SD_PUBLIC_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  } else if (buffer == familyChatLog) {
    appendSdLine(SD_SECONDARY_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  } else if (buffer == directChatLog) {
    appendSdLine(SD_DIRECT_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  }
}

static void appendPacketEvent(const char* line) {
  appendLine(packetLog, PACKET_LOG_SIZE, line);
}

static void copyPreferenceString(const char* key, char* dest, size_t destSize, const char* fallback) {
  String value = prefs.getString(key, fallback);
  value.trim();
  if (!value.length()) value = fallback;
  strlcpy(dest, value.c_str(), destSize);
}

static void loadInterfaceSettings() {
  bool needsSeed = !prefs.isKey("devName");
  copyPreferenceString("devName", interfaceDeviceName, sizeof(interfaceDeviceName), DEVICE_NAME);
  copyPreferenceString("hostname", interfaceHostname, sizeof(interfaceHostname), DEVICE_HOSTNAME);
  copyPreferenceString("apSsid", interfaceApSsid, sizeof(interfaceApSsid), INTERFACE_AP_SSID);
  copyPreferenceString("apPass", interfaceApPass, sizeof(interfaceApPass), INTERFACE_AP_PASS);
  copyPreferenceString("webUser", webUiUser, sizeof(webUiUser), WEBUI_AUTH_USER);
  copyPreferenceString("webPass", webUiPass, sizeof(webUiPass), WEBUI_AUTH_PASS);
  interfaceApChannel = prefs.getUChar("apChannel", WIFI_AP_CHANNEL);
  if (interfaceApChannel < 1 || interfaceApChannel > 13) interfaceApChannel = WIFI_AP_CHANNEL;
  bool savedGenericSetup =
    strcmp(interfaceDeviceName, SETUP_DEVICE_NAME) == 0 &&
    strcmp(webUiUser, SETUP_AUTH_USER) == 0 &&
    strcmp(webUiPass, SETUP_AUTH_PASS) == 0;
  if (FORCE_INTERFACE_SETTINGS ||
      (savedGenericSetup &&
      (strcmp(DEVICE_NAME, SETUP_DEVICE_NAME) != 0 ||
       strcmp(WEBUI_AUTH_PASS, SETUP_AUTH_PASS) != 0 ||
       strcmp(INTERFACE_AP_PASS, SETUP_AUTH_PASS) != 0))) {
    strlcpy(interfaceDeviceName, DEVICE_NAME, sizeof(interfaceDeviceName));
    strlcpy(interfaceHostname, DEVICE_HOSTNAME, sizeof(interfaceHostname));
    strlcpy(interfaceApSsid, INTERFACE_AP_SSID, sizeof(interfaceApSsid));
    strlcpy(interfaceApPass, INTERFACE_AP_PASS, sizeof(interfaceApPass));
    strlcpy(webUiUser, WEBUI_AUTH_USER, sizeof(webUiUser));
    strlcpy(webUiPass, WEBUI_AUTH_PASS, sizeof(webUiPass));
    interfaceApChannel = WIFI_AP_CHANNEL;
    needsSeed = true;
  }
  if (needsSeed) {
    prefs.putString("devName", interfaceDeviceName);
    prefs.putString("hostname", interfaceHostname);
    prefs.putString("apSsid", interfaceApSsid);
    prefs.putString("apPass", interfaceApPass);
    prefs.putString("webUser", webUiUser);
    prefs.putString("webPass", webUiPass);
    prefs.putUChar("apChannel", interfaceApChannel);
  }
}

static bool usingDefaultWebCredentials() {
  return strcmp(webUiUser, SETUP_AUTH_USER) == 0 && strcmp(webUiPass, SETUP_AUTH_PASS) == 0;
}

static bool setupRouteAllowed() {
  String uri = server.uri();
  return uri == "/" || uri == "/status" || uri == "/interface" || uri == "/assets/leaflet-1.9.4.js" || uri == "/assets/leaflet-1.9.4.css";
}

static void saveInterfaceSettings(const String& deviceName, const String& hostname, const String& apSsid, uint8_t apChannel, const String& user, const String& pass) {
  String cleanDeviceName = deviceName;
  String cleanHostname = hostname;
  String cleanApSsid = apSsid;
  cleanDeviceName.trim();
  cleanHostname.trim();
  cleanApSsid.trim();
  if (cleanDeviceName.length()) {
    strlcpy(interfaceDeviceName, cleanDeviceName.c_str(), sizeof(interfaceDeviceName));
    prefs.putString("devName", interfaceDeviceName);
  }
  if (!cleanHostname.length()) cleanHostname = interfaceDeviceName;
  strlcpy(interfaceHostname, cleanHostname.c_str(), sizeof(interfaceHostname));
  prefs.putString("hostname", interfaceHostname);
  if (!cleanApSsid.length()) cleanApSsid = interfaceDeviceName;
  strlcpy(interfaceApSsid, cleanApSsid.c_str(), sizeof(interfaceApSsid));
  prefs.putString("apSsid", interfaceApSsid);
  interfaceApChannel = constrain(apChannel, (uint8_t)1, (uint8_t)13);
  prefs.putUChar("apChannel", interfaceApChannel);
  if (user.length()) {
    strlcpy(webUiUser, user.c_str(), sizeof(webUiUser));
    prefs.putString("webUser", webUiUser);
  }
  if (pass.length()) {
    strlcpy(interfaceApPass, pass.c_str(), sizeof(interfaceApPass));
    prefs.putString("apPass", interfaceApPass);
    strlcpy(webUiPass, pass.c_str(), sizeof(webUiPass));
    prefs.putString("webPass", webUiPass);
  }
}

static void startWifiAp() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPsetHostname(interfaceHostname);
  bool ok = WiFi.softAP(interfaceApSsid, interfaceApPass, interfaceApChannel);
  if (ok) {
    server.begin();
    wifiEnabled = true;
    wifiStartedMs = millis();
    appendLine(eventLog, LOG_SIZE, "[wifi] AP enabled\n");
  } else {
    wifiEnabled = false;
    appendLine(eventLog, LOG_SIZE, "[wifi] AP start failed\n");
  }
}

static void startWifiLocal() {
  if (!wifiLocalSsid[0]) {
    wifiEnabled = false;
    appendLine(eventLog, LOG_SIZE, "[wifi] Local SSID not set\n");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(interfaceHostname);
  WiFi.begin(wifiLocalSsid, wifiLocalPass);
  server.begin();
  wifiEnabled = true;
  wifiStartedMs = millis();
  appendLine(eventLog, LOG_SIZE, "[wifi] Local WiFi connecting\n");
}

static void startWifi() {
  if (wifiApMode) startWifiAp();
  else startWifiLocal();
}

static void stopWifi() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiEnabled = false;
  wifiStoppedMs = millis();
  wifiScanActive = false;
  wifiScanRequested = false;
  appendLine(eventLog, LOG_SIZE, "[wifi] radio disabled\n");
}

static void setWifiEnabled(bool enabled) {
  if (enabled == wifiEnabled) return;
  wifiToggleCount++;
  if (enabled) startWifi();
  else stopWifi();
}

static void setWifiApMode(bool apMode) {
  if (wifiApMode == apMode) return;
  bool wasEnabled = wifiEnabled;
  if (wasEnabled) stopWifi();
  wifiApMode = apMode;
  prefs.putBool("wifiApMode", wifiApMode);
  appendLine(eventLog, LOG_SIZE, wifiApMode ? "[wifi] mode set to AP\n" : "[wifi] mode set to Local\n");
  if (wasEnabled) startWifi();
}

static void saveWifiCredentials() {
  if (taWifiSsid) strlcpy(wifiLocalSsid, lv_textarea_get_text(taWifiSsid), sizeof(wifiLocalSsid));
  if (taWifiPass) strlcpy(wifiLocalPass, lv_textarea_get_text(taWifiPass), sizeof(wifiLocalPass));
  prefs.putString("wifiSsid", wifiLocalSsid);
  prefs.putString("wifiPass", wifiLocalPass);
  appendLine(eventLog, LOG_SIZE, "[wifi] Local credentials saved\n");
  if (wifiEnabled && !wifiApMode) {
    stopWifi();
    startWifi();
  }
}

static void renderWifiScanResults(int16_t status) {
  if (!listWifiScan) return;
  lv_obj_clean(listWifiScan);
  wifiScanResultCount = 0;
  if (status == WIFI_SCAN_RUNNING) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scanning...");
    return;
  }
  if (status == WIFI_SCAN_FAILED || status <= 0) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, status == 0 ? "No networks found" : "Scan failed");
    return;
  }

  wifiScanResultCount = min((size_t)status, WIFI_SCAN_MAX_RESULTS);
  if (lblWifiScanStatus) {
    char statusText[48];
    snprintf(statusText, sizeof(statusText), "%u network%s", (unsigned)wifiScanResultCount, wifiScanResultCount == 1 ? "" : "s");
    lv_label_set_text(lblWifiScanStatus, statusText);
  }
  for (size_t i = 0; i < wifiScanResultCount; i++) {
    strlcpy(wifiScanSsids[i], WiFi.SSID(i).c_str(), sizeof(wifiScanSsids[i]));
    wifiScanRssi[i] = WiFi.RSSI(i);
    lv_obj_t* btn = lv_btn_create(listWifiScan);
    lv_obj_set_size(btn, lv_pct(100), 42);
    styleDarkObject(btn, COLOR_PANEL);
    styleDarkBorder(btn, 0x39638D);
    lv_obj_t* label = lv_label_create(btn);
    char itemText[64];
    snprintf(itemText, sizeof(itemText), "%s  %ld dBm", wifiScanSsids[i], (long)wifiScanRssi[i]);
    lv_label_set_text(label, itemText);
    styleBoundedLabel(label, SCREEN_W - 36, COLOR_TEXT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
      size_t index = (size_t)lv_event_get_user_data(e);
      if (index >= wifiScanResultCount) return;
      ensureWifiLocalPage();
      if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiScanSsids[index]);
      setWifiApMode(false);
      showPage(pageWifiLocal);
      if (taWifiPass) {
        lv_textarea_set_text(taWifiPass, "");
        lv_event_send(taWifiPass, LV_EVENT_FOCUSED, nullptr);
      }
    }, LV_EVENT_CLICKED, (void*)i);
  }
  WiFi.scanDelete();
}

static void renderStoredWifiScanResults(int16_t status) {
  if (!listWifiScan) return;
  lv_obj_clean(listWifiScan);
  if (status == WIFI_SCAN_FAILED || status <= 0 || wifiScanResultCount == 0) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, status == 0 ? "No networks found" : "Scan failed");
    return;
  }

  if (lblWifiScanStatus) {
    char statusText[48];
    snprintf(statusText, sizeof(statusText), "%u network%s", (unsigned)wifiScanResultCount, wifiScanResultCount == 1 ? "" : "s");
    lv_label_set_text(lblWifiScanStatus, statusText);
  }

  for (size_t i = 0; i < wifiScanResultCount; i++) {
    lv_obj_t* btn = lv_btn_create(listWifiScan);
    lv_obj_set_size(btn, lv_pct(100), 42);
    styleDarkObject(btn, COLOR_PANEL);
    styleDarkBorder(btn, 0x39638D);
    lv_obj_t* label = lv_label_create(btn);
    char itemText[64];
    snprintf(itemText, sizeof(itemText), "%s  %ld dBm", wifiScanSsids[i], (long)wifiScanRssi[i]);
    lv_label_set_text(label, itemText);
    styleBoundedLabel(label, SCREEN_W - 36, COLOR_TEXT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
      size_t index = (size_t)lv_event_get_user_data(e);
      if (index >= wifiScanResultCount) return;
      ensureWifiLocalPage();
      if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiScanSsids[index]);
      setWifiApMode(false);
      showPage(pageWifiLocal);
      if (taWifiPass) {
        lv_textarea_set_text(taWifiPass, "");
        lv_event_send(taWifiPass, LV_EVENT_FOCUSED, nullptr);
      }
    }, LV_EVENT_CLICKED, (void*)i);
  }
}

static void wifiScanTask(void*) {
  int16_t status = WIFI_SCAN_FAILED;
  size_t resultCount = 0;
  Serial.println("[wifi] task scan starting");
  WiFi.mode(WIFI_STA);
  WiFi.scanDelete();
  status = WiFi.scanNetworks(false, false, false, 180);
  if (status > 0) {
    resultCount = min((size_t)status, WIFI_SCAN_MAX_RESULTS);
    for (size_t i = 0; i < resultCount; i++) {
      strlcpy(wifiScanSsids[i], WiFi.SSID(i).c_str(), sizeof(wifiScanSsids[i]));
      wifiScanRssi[i] = WiFi.RSSI(i);
    }
  }
  WiFi.scanDelete();
  wifiScanResultCount = resultCount;
  wifiScanTaskStatus = status;
  wifiScanTaskDone = true;
  wifiScanTaskRunning = false;
  Serial.printf("[wifi] task scan complete status=%d count=%u\n", status, (unsigned)resultCount);
  vTaskDelete(nullptr);
}

static void requestWifiScan() {
  wifiScanRequested = true;
  wifiScanActive = false;
  if (listWifiScan) lv_obj_clean(listWifiScan);
  if (lblWifiScanStatus) {
    lv_label_set_text(lblWifiScanStatus, "Scan requested...");
  }
}

static void startWifiScan() {
  if (wifiScanActive || !wifiScanRequested) return;
  wifiScanRequested = false;
  wifiScanActive = true;
  renderWifiScanResults(WIFI_SCAN_RUNNING);
  wifiScanStartedMs = millis();
  wifiScanTaskDone = false;
  wifiScanTaskStatus = WIFI_SCAN_RUNNING;
  wifiScanResultCount = 0;
  if (wifiScanTaskRunning) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Previous scan still running");
    wifiScanActive = false;
    return;
  }
  wifiScanTaskRunning = true;
  BaseType_t ok = xTaskCreatePinnedToCore(wifiScanTask, "wifiScan", 8192, nullptr, 1, nullptr, 0);
  Serial.printf("[wifi] scan task create=%ld\n", (long)ok);
  if (ok != pdPASS) {
    wifiScanTaskRunning = false;
    wifiScanActive = false;
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scan task failed");
  }
}

static void pollWifiScan() {
  if (wifiScanRequested && wifiEnabled && !wifiScanStoppedWifi) {
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiEnabled = false;
    wifiStoppedMs = millis();
    wifiScanStoppedWifi = true;
    wifiScanRequestedMs = millis();
    appendLine(eventLog, LOG_SIZE, "[wifi] disabled for scan\n");
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Starting scan...");
    return;
  }
  if (wifiScanRequested && millis() - wifiScanRequestedMs >= 750) {
    startWifiScan();
  }
  if (!wifiScanActive) return;
  if (wifiScanTaskDone) {
    wifiScanActive = false;
    wifiScanTaskDone = false;
    renderStoredWifiScanResults(wifiScanTaskStatus);
    return;
  }
  if (millis() - wifiScanStartedMs > 9000) {
    wifiScanActive = false;
    Serial.println("[wifi] task scan timeout");
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scan timed out");
  }
}

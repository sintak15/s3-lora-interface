// Internal implementation module; included once by the main sketch.
static const char* linkHealthText() {
  return FieldStatus::linkState(radioResponseSeen, lastRadioResponseMs, millis());
}

static const char* shortLinkHealthText() {
  if (!radioResponseSeen) return "Heltec waiting";
  uint32_t age = millis() - lastRadioResponseMs;
  if (age >= 90000) return "Heltec stale";
  return age >= 45000 ? "Heltec quiet" : "Heltec OK";
}

static uint32_t loraStatusColor() {
  if (!radioResponseSeen || millis() - lastRadioResponseMs >= 90000) return 0xFF5A5F;
  return millis() - lastRadioResponseMs >= 45000 ? 0xFFD166 : COLOR_ACTION;
}

static void printFirmwareIdentity() {
  Serial.printf("[version] firmware=%s commit=%s dirty=%s built=%s %s partition=%s\n",
                FIRMWARE_VERSION, FIRMWARE_GIT_COMMIT, FIRMWARE_SOURCE_DIRTY ? "true" : "false",
                __DATE__, __TIME__, esp_ota_get_running_partition()->label);
}

static void formatBatteryRuntime(char* out, size_t size) {
  if (!localBattery.gaugePresent) strlcpy(out, "unavailable (no gauge)", size);
  else if (localBattery.charging) strlcpy(out, "charging", size);
  else if (batteryRuntime.minutes() < 0) strlcpy(out, "estimating...", size);
  else {
    int32_t minutes = FieldStatus::roundedRuntimeMinutes(batteryRuntime.minutes());
    const char* basis = batteryRuntime.basis();
    const char* label = strcmp(basis, "previous") == 0 ? "prior use" :
                        strcmp(basis, "gauge") == 0 ? "gauge trend" : "current use";
    snprintf(out, size, "~%ldh %ldm (%s)", (long)(minutes / 60), (long)(minutes % 60), label);
  }
}

static bool gpsSatsPresent() {
  if (localGpsSatellitesInView() > 0) return true;
  if (localGps.satellites.isValid() && localGps.satellites.age() <= 30000 && localGps.satellites.value() > 0) return true;
  if (!gpsStats.hasSats || gpsStats.sats == 0 || gpsStats.lastUpdateMs == 0) return false;
  return millis() - gpsStats.lastUpdateMs <= 300000UL;
}

static void formatPreviewLine(const char* name, uint16_t unread, const char* preview, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  const char* body = preview && preview[0] ? preview : "No messages";
  if (unread) snprintf(out, outSize, "%s %u new: %s", name, unread, body);
  else snprintf(out, outSize, "%s: %s", name, body);
}

static void refreshDashboardLabels() {
  char line[160];
  if (lblHomeLink) {
    const char* state = linkHealthText();
    lv_label_set_text(lblHomeLink, !radioResponseSeen ? "Waiting" : strcmp(state, "responsive") == 0 ? "Connected" : strcmp(state, "quiet") == 0 ? "Quiet" : "No response");
    if (lblHomeLinkDetail) {
      if (!radioResponseSeen) strlcpy(line, "Listening for Heltec", sizeof(line));
      else {
        uint32_t age = (millis() - lastRadioResponseMs) / 1000;
        if (age < 60) snprintf(line, sizeof(line), "Last heard %lus ago", (unsigned long)age);
        else if (age < 3600) snprintf(line, sizeof(line), "Last heard %lum ago", (unsigned long)(age / 60));
        else snprintf(line, sizeof(line), "Last heard %luh ago", (unsigned long)(age / 3600));
      }
      lv_label_set_text(lblHomeLinkDetail, line);
    }
    if (homeLinkIcon) lv_obj_set_style_text_color(homeLinkIcon, lv_color_hex(radioResponseSeen ? loraStatusColor() : 0xCDB681), 0);
  }
  if (lblHomeGps) {
    bool fresh = MapNavigation::fresh(gpsStats.valid, gpsStats.lastUpdateMs, millis());
    lv_label_set_text(lblHomeGps, fresh ? "GPS ready" : (gpsSatsPresent() ? "Finding GPS" : "GPS waiting"));
    lv_obj_set_style_text_color(lblHomeGps, lv_color_hex(fresh ? 0xA9D4B7 : 0xA8BCB0), 0);
  }
  if (lblHomeNodes) {
    snprintf(line, sizeof(line), "%u", (unsigned)nodeCount);
    lv_label_set_text(lblHomeNodes, line);
  }
  if (lblHomeMessageTitle) {
    uint16_t unreadTotal = unreadPublic + unreadFamily + unreadDirect;
    lv_label_set_text(lblHomeMessageTitle, "Messages");
    if (lblHomeUnread) {
      if (unreadTotal) snprintf(line, sizeof(line), "%u new", unreadTotal);
      else line[0] = 0;
      lv_label_set_text(lblHomeUnread, line);
    }
  }
  if (lblHomeMessages) lv_label_set_text(lblHomeMessages, previewLatest[0] ? previewLatest : "No messages yet");
  if (lblHomePower) {
    if (!localBattery.gaugePresent) strlcpy(line, "--", sizeof(line));
    else snprintf(line, sizeof(line), "%d%%", localBattery.percent);
    lv_label_set_text(lblHomePower, line);
    uint32_t color = !localBattery.gaugePresent ? 0xA8BCB0 : localBattery.batteryCritical ? 0xF29A91 : localBattery.batteryLow ? 0xE2C17E : 0xEDF5EF;
    lv_obj_set_style_text_color(lblHomePower, lv_color_hex(color), 0);
    if (lblHomePowerDetail) {
      if (!localBattery.gaugePresent) strlcpy(line, "No gauge", sizeof(line));
      else if (localBattery.charging) strlcpy(line, "Charging", sizeof(line));
      else if (localBattery.batteryCritical) strlcpy(line, "Charge soon", sizeof(line));
      else if (localBattery.batteryLow) strlcpy(line, "Battery low", sizeof(line));
      else formatBatteryRuntime(line, sizeof(line));
      lv_label_set_text(lblHomePowerDetail, line);
    }
    if (homePowerBar) {
      if (localBattery.gaugePresent) lv_obj_clear_flag(homePowerBar, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(homePowerBar, LV_OBJ_FLAG_HIDDEN);
      lv_bar_set_value(homePowerBar, constrain(localBattery.percent, 0, 100), LV_ANIM_OFF);
      lv_obj_set_style_bg_color(homePowerBar, lv_color_hex(localBattery.batteryLow || localBattery.batteryCritical ? color : 0x8DCEAA), LV_PART_INDICATOR);
    }
  }
  if (lblMsgPublic) {
    formatPreviewLine(publicChannelLabel(), unreadPublic, previewPublic, line, sizeof(line));
    lv_label_set_text(lblMsgPublic, line);
  }
  if (lblMsgFamily) {
    formatPreviewLine(privateChannelLabel(), unreadFamily, previewFamily, line, sizeof(line));
    lv_label_set_text(lblMsgFamily, line);
  }
  if (lblMsgDirect) {
    formatPreviewLine("Direct", unreadDirect, previewDirect, line, sizeof(line));
    lv_label_set_text(lblMsgDirect, line);
  }
}

#include "MapUi.inc"
#include "MapNavigationTests.inc"
#include "MapCoverageTests.inc"

static void refreshScreenUi() {
  if (!lblStatus) return;
  bool pageForce = forcePageRefresh;
  if (!pageForce && millis() - lastUiRefreshMs < 500) return;
  forcePageRefresh = false;
  lastUiRefreshMs = millis();
  refreshSdUsage();

  char status[48];
  if (localGps.time.isValid()) {
    int cdtHour = (localGps.time.hour() + 19) % 24;
    snprintf(status, sizeof(status), "%02d:%02d", cdtHour, localGps.time.minute());
  } else {
    strlcpy(status, "CYD", sizeof(status));
  }
  lv_label_set_text(lblStatus, status);

  if (lblRadioStatus) {
    lv_label_set_text(lblRadioStatus, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(lblRadioStatus, lv_color_hex(loraStatusColor()), 0);
  }

  if (lblGpsStatus) {
    char gpsStatus[16];
    bool satsPresent = gpsSatsPresent();
    if (satsPresent) {
      int visible = localGpsSatellitesInView();
      unsigned long shownSats = visible >= 0 ? (unsigned long)visible : gpsStats.sats;
      if (shownSats > 99) shownSats = 99;
      snprintf(gpsStatus, sizeof(gpsStatus), LV_SYMBOL_GPS " %lu", shownSats);
    } else {
      strlcpy(gpsStatus, LV_SYMBOL_GPS " 0", sizeof(gpsStatus));
    }
    lv_label_set_text(lblGpsStatus, gpsStatus);
    bool fresh = MapNavigation::fresh(gpsStats.valid, gpsStats.lastUpdateMs, millis());
    lv_obj_set_style_text_color(lblGpsStatus, lv_color_hex(fresh ? COLOR_ACTION : satsPresent ? 0xFFD166 : 0xFF5A5F), 0);
  }

  if (lblBatteryStatus) {
    char batteryStatus[32];
    if (localBattery.gaugePresent) {
      if (localBattery.charging) {
        snprintf(batteryStatus, sizeof(batteryStatus), LV_SYMBOL_CHARGE " %d%%", localBattery.percent);
      } else {
        const char* icon = LV_SYMBOL_BATTERY_EMPTY;
        if (localBattery.percent >= 90) icon = LV_SYMBOL_BATTERY_FULL;
        else if (localBattery.percent >= 65) icon = LV_SYMBOL_BATTERY_3;
        else if (localBattery.percent >= 35) icon = LV_SYMBOL_BATTERY_2;
        else if (localBattery.percent >= 12) icon = LV_SYMBOL_BATTERY_1;
        snprintf(batteryStatus, sizeof(batteryStatus), "%s %d%%", icon, localBattery.percent);
      }
    } else {
      strlcpy(batteryStatus, "No gauge", sizeof(batteryStatus));
    }
    lv_label_set_text(lblBatteryStatus, batteryStatus);
    uint32_t batteryColor = localBattery.gaugePresent
                              ? (localBattery.batteryCritical ? 0xFF5A5F :
                                 (localBattery.batteryLow ? 0xFFD166 : COLOR_ACCENT))
                              : COLOR_MUTED;
    lv_obj_set_style_text_color(lblBatteryStatus, lv_color_hex(batteryColor), 0);
  }

  refreshDashboardLabels();

  if (currentPage == pageSystemInterface && lblSystemInterface) {
    String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("off");
    char interfaceText[680];
    snprintf(interfaceText, sizeof(interfaceText),
             "S3 interface %s\nSource: %.12s%s\n"
             "Uptime: %lu s\n"
             "WiFi: %s\n\n"
             "Memory\n"
             "Heap free/min: %lu/%lu KB\n"
             "Internal free/largest: %lu/%lu KB\n"
             "PSRAM free/largest: %lu/%lu KB\n\n"
             "Map render\n"
             "Last/max: %lu/%lu ms\n"
             "Count/draw/cache: %lu/%lu/%lu\n"
             "Worker start/apply/fail: %lu/%lu/%lu\n\n"
             "SD card\n"
             "Status: %s\n"
             "Type: %s\n"
             "Used/total: %lu/%lu MB\n"
             "Writes/errors/busy: %lu/%lu/%lu\n\n"
             "Append wait/ok/drop: %u/%lu/%lu\n"
             "Append depth/fallback: %u/%lu\n\n"
             "UI\n"
             "Frames decoded: %lu",
             FIRMWARE_VERSION, FIRMWARE_GIT_COMMIT, FIRMWARE_SOURCE_DIRTY ? " modified" : "",
             (unsigned long)(millis() / 1000),
              wifiIp.c_str(),
             (unsigned long)(ESP.getFreeHeap() / 1024),
             (unsigned long)(ESP.getMinFreeHeap() / 1024),
             heapCapKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heapCapLargestKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heapCapKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             heapCapLargestKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned long)lastMapRenderDurationMs,
             (unsigned long)maxMapRenderDurationMs,
             (unsigned long)mapRenderCount,
             (unsigned long)mapRenderDraws,
             (unsigned long)mapRenderCacheHits,
             (unsigned long)mapRenderWorkerStarts,
             (unsigned long)mapRenderWorkerApplies,
             (unsigned long)mapRenderWorkerFailures,
             sdStorage.status,
             sdStorage.cardType,
             bytesToWholeMb(sdStorage.usedBytes),
             bytesToWholeMb(sdStorage.totalBytes),
             (unsigned long)sdStorage.writes,
             (unsigned long)sdStorage.writeErrors,
             (unsigned long)sdLockTimeouts,
             (unsigned)sdAppendQueueWaiting(),
             (unsigned long)sdAppendWritten,
             (unsigned long)sdAppendDropped,
             (unsigned)SD_APPEND_QUEUE_DEPTH,
             (unsigned long)sdAppendFallbacks,
             (unsigned long)framesDecoded);
    lv_label_set_text(lblSystemInterface, interfaceText);
  }

  if (currentPage == pageSystemSerial && lblSystemSerial) {
    char serialText[420];
    char rxAge[32];
    if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lus ago", (unsigned long)((millis() - lastByteMs) / 1000));
    else strlcpy(rxAge, "never", sizeof(rxAge));
    snprintf(serialText, sizeof(serialText),
             "Heltec: %s\n"
             "Last valid response: %s\n"
             "Serial link\n"
             "RX bytes: %lu\n"
             "TX bytes: %lu\n"
             "Last byte: %s\n"
             "Magic 94/C3: %lu/%lu\n\n"
             "Stream\n"
             "Frames: %lu\n"
             "Decode errors: %lu\n"
             "Bad lengths: %lu\n\n"
             "API keepalive\n"
             "Heartbeats: %lu\n"
             "Refreshes: %lu\n\n"
             "ASCII seen\n"
             "%.48s",
             linkHealthText(),
             radioResponseSeen ? String((millis() - lastRadioResponseMs) / 1000).c_str() : "never",
             (unsigned long)bytesFromRadio,
             (unsigned long)bytesToRadio,
             rxAge,
             (unsigned long)magic1Count,
             (unsigned long)magic2Count,
             (unsigned long)streamFrames,
             (unsigned long)decodeErrors,
             (unsigned long)invalidFrameLengths,
             (unsigned long)heartbeatCount,
             (unsigned long)apiResyncCount,
             serialPeek);
    lv_label_set_text(lblSystemSerial, serialText);
  }

  if (currentPage == pageSystemRadio && lblSystemRadio) {
    char radioText[300];
    snprintf(radioText, sizeof(radioText),
             "Packet types\n"
             "Text: %lu\n"
             "Telemetry: %lu\n"
             "GPS: %lu\n"
             "Node info: %lu\n"
             "Remote GPS mapped: %lu\n\n"
             "Other traffic\n"
             "Config: %lu\n"
             "Other: %lu\n"
             "Encrypted: %lu\n"
             "Last port: %lu",
             (unsigned long)textPackets,
             (unsigned long)telemetryPackets,
             (unsigned long)positionPackets,
             (unsigned long)nodeInfoPackets,
             (unsigned long)remotePositionPackets,
             (unsigned long)configFrames,
             (unsigned long)otherFrames,
             (unsigned long)encryptedPackets,
             (unsigned long)lastPortNum);
    lv_label_set_text(lblSystemRadio, radioText);
  }

  if (currentPage == pageNodes && (pageForce || isUiDirty(UI_DIRTY_NODES))) {
    refreshNodeList(pageForce);
  }

  if (currentPage == pageNodeDetail && taNodeDetail) {
    NodeRecord* node = findNode(selectedNodeNum);
    char detailText[960];
    if (node) {
      uint32_t now = millis();
      uint32_t ageSeconds = nodeAgeSeconds(*node, now);
      char rangeText[32];
      formatRangeBearing(*node, rangeText, sizeof(rangeText));
      char positionAgeText[24];
      if (node->hasPosition) snprintf(positionAgeText, sizeof(positionAgeText), "%lu s", (unsigned long)((now - node->lastPositionMs) / 1000));
      else strlcpy(positionAgeText, "--", sizeof(positionAgeText));
      char positionText[160];
      if (node->hasPosition) {
        snprintf(positionText, sizeof(positionText),
                 "Map: %.5f, %.5f\nAlt %ld m | pos age %s\n%s",
                 node->latitude,
                 node->longitude,
                 (long)node->altitude,
                 positionAgeText,
                 rangeText);
      } else {
        strlcpy(positionText, "Map: no node position yet\nMap opens on current/last CYD location", sizeof(positionText));
      }
      char telemetryText[112];
      if (node->hasDeviceMetrics) {
        snprintf(telemetryText, sizeof(telemetryText),
                 "Power: %lu%% %.2fV | age %lu s",
                 (unsigned long)node->batteryLevel,
                 node->voltage,
                 (unsigned long)((now - node->lastTelemetryMs) / 1000));
      } else {
        strlcpy(telemetryText, "Power: no telemetry yet", sizeof(telemetryText));
      }
      snprintf(detailText, sizeof(detailText),
               "%.39s\n"
               "!%08lX\n\n"
               "%s\n"
               "%s | heard %lu s\n"
               "Signal: %.1f dB / %ld dBm | hops %u\n"
               "Channel: %u %s | port %lu\n"
               "Packets: %lu total\n\n"
               "%s\n\n"
               "%s\n\n"
               "Text/Tel/Pos/Enc: %lu/%lu/%lu/%lu",
               node->name,
               (unsigned long)node->num,
               node->isFavorite ? "Favorite node" : "Regular node",
               nodeStatusLabel(*node, now),
               (unsigned long)ageSeconds,
               node->snr,
               (long)node->rssi,
               node->hopsAway,
               node->lastChannel,
               channelName(node->lastChannel),
               (unsigned long)node->lastPortNum,
               (unsigned long)node->packetsHeard,
               positionText,
               telemetryText,
               (unsigned long)node->textPackets,
               (unsigned long)node->telemetryPackets,
               (unsigned long)node->positionPackets,
               (unsigned long)node->encryptedPackets);
    } else {
      snprintf(detailText, sizeof(detailText), "Select a node from the node list.");
    }
    bool resetScroll = nodeDetailScrollTopPending || nodeDetailRenderedNode != selectedNodeNum;
    bool textChanged = strcmp(nodeDetailLastText, detailText) != 0;
    lv_coord_t scrollY = resetScroll ? 0 : lv_obj_get_scroll_y(taNodeDetail);
    if (textChanged) {
      lv_textarea_set_text(taNodeDetail, detailText);
      strlcpy(nodeDetailLastText, detailText, sizeof(nodeDetailLastText));
    }
    if (textChanged || resetScroll) {
      lv_textarea_set_cursor_pos(taNodeDetail, 0);
      lv_obj_update_layout(taNodeDetail);
      lv_obj_scroll_to_y(taNodeDetail, scrollY, LV_ANIM_OFF);
    }
    nodeDetailRenderedNode = selectedNodeNum;
    nodeDetailScrollTopPending = false;
    refreshNodeDetailFavoriteButton();
  }

  if (currentPage == pageMeshHealth && lblMeshHealth) {
    uint32_t now = millis();
    size_t activeNodes = 0;
    size_t staleNodes = 0;
    const NodeRecord* best = nullptr;
    const NodeRecord* weakest = nullptr;
    for (size_t i = 0; i < nodeCount; i++) {
      const NodeRecord& node = nodes[i];
      uint32_t age = nodeAgeSeconds(node, now);
      if (age <= 900) activeNodes++;
      else staleNodes++;
      if (!best || node.snr > best->snr) best = &node;
      if (!weakest || node.snr < weakest->snr) weakest = &node;
    }
    char rxAge[32];
    if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lu s ago", (unsigned long)((now - lastByteMs) / 1000));
    else strlcpy(rxAge, "never", sizeof(rxAge));
    TxRecord* lastTx = latestTxRecord();
    char txDest[24] = "-";
    const char* txStatus = "none";
    uint32_t txAge = 0;
    uint8_t txChannel = 0;
    if (lastTx) {
      describeTxDestination(*lastTx, txDest, sizeof(txDest));
      txStatus = txStatusText(*lastTx, now);
      txAge = (now - lastTx->sentMs) / 1000;
      txChannel = lastTx->channel;
    }
    char healthText[760];
    snprintf(healthText, sizeof(healthText),
             "Mesh\n"
             "Known: %u  Active: %u\n"
             "Stale: %u  Positioned: %u\n"
             "Radio online/total: %u/%u\n\n"
             "Traffic\n"
             "Frames: %lu  Errors: %lu\n"
             "RX/TX packets: %lu/%lu\n"
             "Heltec: %s  valid response %s s ago\n"
             "Bytes RX/TX: %lu/%lu\n"
             "Last byte: %s\n\n"
             "Packet mix\n"
             "Text %lu  Telemetry %lu\n"
             "Position %lu  Node info %lu\n"
             "Config %lu  Other %lu\n"
             "Encrypted %lu\n\n"
             "TX\n"
             "Last: %s %lus\n"
             "Dest: %s  ch%u\n"
             "TX bytes: %lu\n\n"
             "Link quality\n"
             "Best: %s %.1f dB\n"
             "Weak: %s %.1f dB\n"
             "Channel use: %.2f%%\n"
             "Air TX: %.2f%%",
             (unsigned)nodeCount,
             (unsigned)activeNodes,
             (unsigned)staleNodes,
             (unsigned)countPositionedNodes(),
             stats.onlineNodes,
             stats.totalNodes,
             (unsigned long)framesDecoded,
             (unsigned long)decodeErrors,
             (unsigned long)stats.packetsRx,
             (unsigned long)stats.packetsTx,
             linkHealthText(),
             radioResponseSeen ? String((millis() - lastRadioResponseMs) / 1000).c_str() : "never",
             (unsigned long)bytesFromRadio,
             (unsigned long)bytesToRadio,
             rxAge,
             (unsigned long)textPackets,
             (unsigned long)telemetryPackets,
             (unsigned long)positionPackets,
             (unsigned long)nodeInfoPackets,
             (unsigned long)configFrames,
             (unsigned long)otherFrames,
             (unsigned long)encryptedPackets,
             txStatus,
             (unsigned long)txAge,
             txDest,
             txChannel,
             (unsigned long)bytesToRadio,
             best ? best->name : "-",
             best ? best->snr : 0.0f,
             weakest ? weakest->name : "-",
             weakest ? weakest->snr : 0.0f,
             stats.channelUtilization,
             stats.airUtilTx);
    lv_label_set_text(lblMeshHealth, healthText);
  }

  if (swWifiEnabled) {
    if (wifiEnabled) lv_obj_add_state(swWifiEnabled, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiEnabled, LV_STATE_CHECKED);
  }
  if (swWifiApMode) {
    if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiApMode, LV_STATE_CHECKED);
  }

  if (currentPage == pageWifi && lblWifiState) {
    String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("-");
    const char* wifiState = "off";
    if (wifiEnabled && wifiApMode) wifiState = "AP on";
    else if (wifiEnabled && WiFi.status() == WL_CONNECTED) wifiState = "Local connected";
    else if (wifiEnabled) wifiState = "Local connecting";
    char wifiStateText[180];
    snprintf(wifiStateText, sizeof(wifiStateText),
             "Status: %s\nSSID: %s\nIP: %s",
             wifiState,
             wifiEnabled ? (wifiApMode ? interfaceApSsid : wifiLocalSsid) : "-",
             wifiIp.c_str());
    lv_label_set_text(lblWifiState, wifiStateText);
  }

  if (currentPage == pageWifiStats && lblWifiStats) {
    uint32_t wifiAge = 0;
    if (wifiEnabled && wifiStartedMs) wifiAge = (millis() - wifiStartedMs) / 1000;
    else if (!wifiEnabled && wifiStoppedMs) wifiAge = (millis() - wifiStoppedMs) / 1000;

    String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("-");
    String wifiMac = wifiEnabled ? (wifiApMode ? WiFi.softAPmacAddress() : WiFi.macAddress()) : String("-");
    char wifiStatsText[560];
    snprintf(wifiStatsText, sizeof(wifiStatsText),
             "WiFi\n"
             "Mode: %s\n"
             "State: %s\n"
             "SSID: %s\n"
             "IP: %s\n"
             "MAC: %s\n"
             "Stations: %u\n"
             "RSSI: %ld dBm\n"
             "Channel: %d\n\n"
             "Runtime\n"
             "%s for: %lu s\n"
             "Toggles: %lu\n\n"
             "Web server\n"
             "State: %s\n"
             "Port: 80",
             wifiApMode ? "AP" : "Local",
             wifiEnabled ? "on" : "off",
             wifiEnabled ? (wifiApMode ? interfaceApSsid : wifiLocalSsid) : "-",
             wifiIp.c_str(),
             wifiMac.c_str(),
             (wifiEnabled && wifiApMode) ? WiFi.softAPgetStationNum() : 0,
             (wifiEnabled && !wifiApMode && WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0,
             wifiEnabled ? WiFi.channel() : 0,
             wifiEnabled ? "On" : "Off",
             (unsigned long)wifiAge,
             (unsigned long)wifiToggleCount,
             wifiEnabled ? "listening" : "stopped");
    lv_label_set_text(lblWifiStats, wifiStatsText);
  }

  if (sliderBacklight && (uint8_t)lv_slider_get_value(sliderBacklight) != backlightPercent) {
    lv_slider_set_value(sliderBacklight, backlightPercent, LV_ANIM_OFF);
  }
  if (sliderToneVolume && (uint8_t)lv_slider_get_value(sliderToneVolume) != toneVolumePercent) {
    lv_slider_set_value(sliderToneVolume, toneVolumePercent, LV_ANIM_OFF);
  }
  if (dropdownToneMessage || dropdownToneDirect) {
    updateToneDropdowns();
  }
  if (currentPage == pageSound) updateToneVolumeLabel();

  if (currentPage == pageBacklight && lblBacklight) {
    char backlightText[96];
    snprintf(backlightText, sizeof(backlightText),
             "Brightness: %u%%\nPWM: %s",
             backlightPercent,
             backlightPwmReady ? "LEDC" : "analog fallback");
    lv_label_set_text(lblBacklight, backlightText);
  }

  if (currentPage == pageSound && lblToneVolume) {
    updateToneVolumeLabel();
  }

  if (currentPage == pageMessageDiag) {
    refreshMessageDiagView();
  }

  if (currentPage == pageBattery && lblBatteryStats) {
    char batteryText[1100];
    char runtimeText[48];
    formatBatteryRuntime(runtimeText, sizeof(runtimeText));
    uint32_t gaugeAge = localBattery.lastGaugeMs ? (millis() - localBattery.lastGaugeMs) / 1000 : 0;
    if (localBattery.gaugePresent) {
      snprintf(batteryText, sizeof(batteryText),
               "LiPo fuel gauge\n"
               "Chip: MAX17048  v0x%04X\n"
               "Displayed: %d%%\n"
               "Remaining: %s\n"
               "Alert: %s\n"
               "Displayed source: %s\n"
               "Gauge SOC: %.1f%%  raw 0x%04X\n"
               "Pack filtered: %.3f V\n"
               "Pack raw: %.3f V\n"
               "Voltage filter: %s  rejected %lu\n"
               "Voltage low/critical: %s/%s\n"
               "Charge rate: %.1f%%/hr\n"
               "Last change: %ld mV\n"
               "Voltage trend: %.1f mV/min\n"
               "Charging: %s\n"
               "Discharge: %.1f%%/hr\n"
               "Saved discharge: %.1f%%/hr\n"
               "Updated: %lu s ago\n\n"
               "Power path\n"
               "LiPo > charger > booster > 5V pins\n\n"
               "S3 interface\n"
               "Uptime: %lu s\n"
               "Heap free/min: %lu/%lu KB\n"
               "PSRAM free: %lu KB",
               localBattery.gaugeVersion,
               localBattery.percent,
               runtimeText,
               batteryAlertLabel(),
               localBattery.percentSource,
               localBattery.gaugeSoc,
               localBattery.rawGaugeSoc,
               localBattery.batteryMv / 1000.0f,
               localBattery.rawPackMv / 1000.0f,
               localBattery.voltageUnstable ? "settling" : "stable",
               (unsigned long)localBattery.voltageRejectedSamples,
               localBattery.voltageLowSustained ? "yes" : "no",
               localBattery.voltageCriticalSustained ? "yes" : "no",
               localBattery.chargeRateValid ? localBattery.chargeRatePercentHr : 0.0f,
               (long)localBattery.instantDeltaMv,
               localBattery.deltaMvPerMinTenths / 10.0f,
               localBattery.charging ? "yes" : "no",
               batteryRuntime.percentPerHour(),
               localBattery.savedDischargeRate,
               (unsigned long)gaugeAge,
               (unsigned long)(millis() / 1000),
               (unsigned long)(ESP.getFreeHeap() / 1024),
               (unsigned long)(ESP.getMinFreeHeap() / 1024),
               (unsigned long)(ESP.getFreePsram() / 1024));
    } else {
      snprintf(batteryText, sizeof(batteryText),
               "LiPo fuel gauge\n"
               "MAX17048 not detected\n"
               "I2C: SDA GPIO%d  SCL GPIO%d\n"
               "Address: 0x%02X\n\n"
               "Power path\n"
               "LiPo > charger > booster > 5V pins\n\n"
               "S3 interface\n"
               "Uptime: %lu s\n"
               "Heap free/min: %lu/%lu KB\n"
               "PSRAM free: %lu KB",
               TOUCH_SDA,
               TOUCH_SCL,
               MAX17048_ADDR,
               (unsigned long)(millis() / 1000),
               (unsigned long)(ESP.getFreeHeap() / 1024),
               (unsigned long)(ESP.getMinFreeHeap() / 1024),
               (unsigned long)(ESP.getFreePsram() / 1024));
    }
    lv_label_set_text(lblBatteryStats, batteryText);
  }

  if (currentPage == pageSystemGps && lblGpsStats) {
    char gpsText[720];
    if (gpsStats.valid) {
      uint32_t age = (millis() - gpsStats.lastUpdateMs) / 1000;
      char satsText[24];
      char fixQualityText[32];
      char fixTypeText[24];
      char timeText[32];
      char dopText[64];
      char accuracyText[32];
      char speedText[32];
      char trackText[32];
      char nextUpdateText[32];
      char precisionText[32];
      char altExtraText[72];
      if (gpsStats.hasSats) snprintf(satsText, sizeof(satsText), "%lu", (unsigned long)gpsStats.sats);
      else strlcpy(satsText, "not sent", sizeof(satsText));
      if (gpsStats.hasFixQuality) snprintf(fixQualityText, sizeof(fixQualityText), "%lu", (unsigned long)gpsStats.fixQuality);
      else strlcpy(fixQualityText, "not sent", sizeof(fixQualityText));
      if (gpsStats.hasFixType) snprintf(fixTypeText, sizeof(fixTypeText), "%luD", (unsigned long)gpsStats.fixType);
      else strlcpy(fixTypeText, "not sent", sizeof(fixTypeText));
      if (gpsStats.hasTimestamp) snprintf(timeText, sizeof(timeText), "%lu", (unsigned long)gpsStats.timestamp);
      else strlcpy(timeText, "not sent", sizeof(timeText));
      if (gpsStats.hasPdop || gpsStats.hasHdop || gpsStats.hasVdop) {
        snprintf(dopText, sizeof(dopText), "P %.2f  H %.2f  V %.2f",
                 gpsStats.pdop / 100.0f, gpsStats.hdop / 100.0f, gpsStats.vdop / 100.0f);
      } else {
        strlcpy(dopText, "not sent", sizeof(dopText));
      }
      if (gpsStats.hasAccuracy) snprintf(accuracyText, sizeof(accuracyText), "%.1f m", gpsStats.accuracyMm / 1000.0f);
      else strlcpy(accuracyText, "not sent", sizeof(accuracyText));
      if (gpsStats.hasGroundSpeed) snprintf(speedText, sizeof(speedText), "%.1f m/s", gpsStats.groundSpeed / 100.0f);
      else strlcpy(speedText, "not sent", sizeof(speedText));
      if (gpsStats.hasGroundTrack) snprintf(trackText, sizeof(trackText), "%.1f deg", gpsStats.groundTrack / 100.0f);
      else strlcpy(trackText, "not sent", sizeof(trackText));
      if (gpsStats.hasNextUpdate) snprintf(nextUpdateText, sizeof(nextUpdateText), "%lu s", (unsigned long)gpsStats.nextUpdate);
      else strlcpy(nextUpdateText, "not sent", sizeof(nextUpdateText));
      if (gpsStats.hasPrecision) snprintf(precisionText, sizeof(precisionText), "%lu bits", (unsigned long)gpsStats.precisionBits);
      else strlcpy(precisionText, "not sent", sizeof(precisionText));
      char haeText[18];
      char geoidText[18];
      if (gpsStats.hasAltitudeHae) snprintf(haeText, sizeof(haeText), "%ld m", (long)gpsStats.altitudeHae);
      else strlcpy(haeText, "not sent", sizeof(haeText));
      if (gpsStats.hasGeoidalSeparation) snprintf(geoidText, sizeof(geoidText), "%ld m", (long)gpsStats.geoidalSeparation);
      else strlcpy(geoidText, "not sent", sizeof(geoidText));
      snprintf(altExtraText, sizeof(altExtraText), "HAE: %s  Geoid: %s",
               haeText,
               geoidText);
      snprintf(gpsText, sizeof(gpsText),
               "Node: %s\n"
               "Record: %s\n"
               "Latitude:  %.6f\n"
               "Longitude: %.6f\n"
               "Altitude:  %ld m\n"
               "%s\n"
               "Satellites: %s\n"
               "Fix quality: %s\n"
               "Fix type: %s\n"
             "DOP: %s\n"
             "Accuracy: %s\n"
             "Speed: %s\n"
             "Track: %s\n"
             "CYD UART: %lu bytes\n"
             "GPS time: %s\n"
               "Next update: %s\n"
               "Precision: %s\n"
               "Seq/Sensor: %lu/%lu\n"
               "Updated: %lu s ago",
               nodeName(gpsStats.from),
               gpsStats.sourceKind,
               gpsStats.latitude,
               gpsStats.longitude,
               (long)gpsStats.altitude,
               altExtraText,
               satsText,
               fixQualityText,
               fixTypeText,
               dopText,
               accuracyText,
               speedText,
               trackText,
               (unsigned long)gpsBytesFromLocal,
               timeText,
               nextUpdateText,
               precisionText,
               (unsigned long)gpsStats.seqNumber,
               (unsigned long)gpsStats.sensorId,
               (unsigned long)age);
    } else {
      int inView = localGpsSatellitesInView();
      char visible[16], used[16];
      if (inView >= 0) snprintf(visible, sizeof(visible), "%d", inView); else strlcpy(visible, "unknown", sizeof(visible));
      if (localGps.satellites.isValid() && localGps.satellites.age() <= 30000) snprintf(used, sizeof(used), "%lu", (unsigned long)localGps.satellites.value()); else strlcpy(used, "unknown", sizeof(used));
      snprintf(gpsText, sizeof(gpsText),
               "Waiting for GPS...\n\n"
               "CYD local GPS:\n"
               "RX GPIO%d at %lu baud\n"
               "Bytes: %lu\n"
               "Valid NMEA: %lu\n"
               "Checksum errors: %lu\n"
               "Fix sentences: %lu\n"
               "Satellites in view: %s\n"
               "Satellites used: %s\n\n"
               "Heltec GPS can still feed\n"
               "Meshtastic position packets.",
               GPS_RX_PIN,
               (unsigned long)GPS_BAUD,
               (unsigned long)gpsBytesFromLocal,
               (unsigned long)localGps.passedChecksum(),
               (unsigned long)localGps.failedChecksum(),
               (unsigned long)localGps.sentencesWithFix(), visible, used);
    }
    lv_label_set_text(lblGpsStats, gpsText);
  }

  refreshMapUi();

  refreshChatViews(pageForce || isUiDirty(UI_DIRTY_CHAT));
  if (currentPage == pagePacketInspector && taPacketInspector && (pageForce || isUiDirty(UI_DIRTY_PACKET_LOG))) {
    setReadonlyTextStable(taPacketInspector, packetLog[0] ? packetLog : "No packets decoded yet");
    clearUiDirty(UI_DIRTY_PACKET_LOG);
  }
  if (currentPage == pageTxHistory) {
    refreshTxHistoryView();
  }
  if (currentPage == pageSystemSerial && taScreenLog && (pageForce || isUiDirty(UI_DIRTY_EVENT_LOG))) {
    setReadonlyTextStable(taScreenLog, eventLog[0] ? eventLog : "Waiting for radio data");
    clearUiDirty(UI_DIRTY_EVENT_LOG);
  }

  if (currentPage == pageSystemSerial && taScreenNodes && (pageForce || isUiDirty(UI_DIRTY_NODES))) {
    String text;
    for (size_t i = 0; i < nodeCount; i++) {
      text += "!";
      text += String(nodes[i].num, HEX);
      text += "  ";
      text += nodes[i].name;
      text += "\nSNR ";
      text += String(nodes[i].snr, 1);
      text += "  ";
      text += String((millis() - nodes[i].lastHeardMs) / 1000);
      text += "s ago\n\n";
    }
    setReadonlyTextStable(taScreenNodes, text.length() ? text.c_str() : "No nodes heard yet");
    clearUiDirty(UI_DIRTY_NODES);
  }
}

#ifdef FIRMWARE_RUN_TESTS
// Exercise UI formatting/rendering only; this does not simulate battery discharge.
static void runUiSmokeTests() {
  lv_obj_t* savedPage = currentPage;
  lv_obj_t* savedNavigation[8];
  memcpy(savedNavigation, navigationStack, sizeof(savedNavigation));
  uint8_t savedDepth = navigationDepth;
  bool savedGaugePresent = localBattery.gaugePresent;
  bool passed = pageBattery && lblBatteryStats;
  if (passed) {
    showPage(pageBattery, false);
    localBattery.gaugePresent = true;
    refreshScreenUi();
    const char* text = lv_label_get_text(lblBatteryStats);
    bool presentOk = strstr(text, "Remaining:") && strstr(text, "PSRAM free:");
    Serial.printf("[test-ui] Battery gauge present: %s\n", presentOk ? "PASS" : "FAIL");
    passed &= presentOk;
    localBattery.gaugePresent = false;
    forcePageRefresh = true;
    refreshScreenUi();
    text = lv_label_get_text(lblBatteryStats);
    bool missingOk = strstr(text, "MAX17048 not detected") && strstr(text, "PSRAM free:");
    Serial.printf("[test-ui] Battery gauge missing: %s\n", missingOk ? "PASS" : "FAIL");
    passed &= missingOk;
  }
  localBattery.gaugePresent = savedGaugePresent;
  struct ScreenCheck { lv_obj_t* page; lv_obj_t* label; const char* expected; const char* name; };
  const ScreenCheck checks[] = {
    {pageLauncher, lblHomeMessageTitle, "Messages", "Home dashboard"},
    {pageOperate, lblMsgDirect, "Direct", "Messages"},
    {pageMeshHealth, lblMeshHealth, "Heltec:", "Mesh Health"},
    {pageSystemInterface, lblSystemInterface, FIRMWARE_VERSION, "System Interface"},
    {pageSystemSerial, lblSystemSerial, "Last valid response:", "System Serial"},
    {pageGps, lblMapZoom, "Zoom", "Map view"},
  };
  for (const auto& check : checks) {
    bool ok = check.page && check.label;
    if (ok) {
      showPage(check.page, false);
      refreshScreenUi();
      ok = strstr(lv_label_get_text(check.label), check.expected) != nullptr;
    }
    Serial.printf("[test-ui] %s: %s\n", check.name, ok ? "PASS" : "FAIL");
    passed &= ok;
  }
  showPage(pageLauncher, false);
  refreshScreenUi();
  lv_obj_update_layout(pageLauncher);
  lv_area_t homeArea;
  lv_obj_get_coords(pageLauncher, &homeArea);
  bool layoutOk = true;
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(pageLauncher); ++i) {
    lv_obj_t* child = lv_obj_get_child(pageLauncher, i);
    lv_area_t area;
    lv_obj_get_coords(child, &area);
    layoutOk &= area.x1 >= homeArea.x1 && area.x2 <= homeArea.x2 && area.y1 >= homeArea.y1 && area.y2 <= homeArea.y2;
    if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) layoutOk &= lv_obj_get_height(child) >= 44;
  }
  bool menuOk = btnNavBack && btnNavHome && strcmp(lv_label_get_text(lv_obj_get_child(btnNavBack, 0)), "Menu") == 0;
  if (menuOk) {
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    menuOk = currentPage == pageFunctions && !lv_obj_has_flag(pageFunctions, LV_OBJ_FLAG_HIDDEN);
    lv_obj_update_layout(pageFunctions);
    unsigned menuActions = 0;
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(pageFunctions); ++i) {
      lv_obj_t* child = lv_obj_get_child(pageFunctions, i);
      if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
        ++menuActions;
        menuOk &= lv_obj_get_height(child) >= 44;
      }
    }
    menuOk &= menuActions == 5;
    // Exercise the nested System > Health route and its Back chain.
    showPage(pageSystem);
    showPage(pageDiagnose);
    showPage(pageMeshHealth);
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    menuOk &= currentPage == pageDiagnose;
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    menuOk &= currentPage == pageSystem;
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    menuOk &= currentPage == pageFunctions;
    lv_event_send(btnNavHome, LV_EVENT_CLICKED, nullptr);
    menuOk &= currentPage == pageLauncher;
  }
  Serial.printf("[test-ui] Launcher menu navigation: %s\n", menuOk ? "PASS" : "FAIL");
  passed &= menuOk;
  Serial.printf("[test-ui] Home bounds and touch targets: %s\n", layoutOk ? "PASS" : "FAIL");
  passed &= layoutOk;
  memcpy(navigationStack, savedNavigation, sizeof(savedNavigation));
  navigationDepth = savedDepth;
  showPage(savedPage, false);
  refreshScreenUi();
  Serial.printf("[test-ui] complete: %s\n", passed ? "PASS" : "FAIL");
}
#endif

static uint32_t parseNodeAddress(const char* text) {
  if (!text) return 0;
  while (*text == ' ' || *text == '\t' || *text == '!') text++;
  if (!*text) return 0;
  char* end = nullptr;
  uint32_t value = strtoul(text, &end, 16);
  if (end == text || value == 0 || value == BROADCAST_ADDR) return 0;
  return value;
}

static void sendDirectFromInput(lv_obj_t* toInput, lv_obj_t* msgInput) {
  if (!toInput || !msgInput) return;
  uint32_t to = parseNodeAddress(lv_textarea_get_text(toInput));
  const char* msg = lv_textarea_get_text(msgInput);
  if (to && msg && msg[0] && sendDirectTextMessage(msg, to)) {
    lv_textarea_set_text(msgInput, "");
  }
}

static void serviceScreen() {
  serviceMapNavigationSettings();
  serviceToneAudio();
  serviceMapRenderWorker();
  processDeferredWifiAction();
  pollWifiScan();
  sampleLocalBattery();
  refreshScreenUi();
  lv_timer_handler();
}

static void printSerialDiagnostics() {
  if (millis() - lastSerialDiagMs < 1000) return;
  lastSerialDiagMs = millis();

  uint32_t age = lastByteMs ? (millis() - lastByteMs) / 1000 : 0xFFFFFFFF;
  Serial.printf(
    "[LINK] health=%s response=%lus rx=%lu tx=%lu last=%s frames=%lu decoded=%lu err=%lu magic=%lu/%lu badlen=%lu hb=%lu resync=%lu ascii=\"%.60s\"\n",
    linkHealthText(),
    (unsigned long)(radioResponseSeen ? (millis() - lastRadioResponseMs) / 1000 : UINT32_MAX),
    (unsigned long)bytesFromRadio,
    (unsigned long)bytesToRadio,
    lastByteMs ? String(age).c_str() : "never",
    (unsigned long)streamFrames,
    (unsigned long)framesDecoded,
    (unsigned long)decodeErrors,
    (unsigned long)magic1Count,
    (unsigned long)magic2Count,
    (unsigned long)invalidFrameLengths,
    (unsigned long)heartbeatCount,
    (unsigned long)apiResyncCount,
    serialPeek
  );
  Serial.printf(
    "[PKT] text=%lu telemetry=%lu gps=%lu node=%lu remoteGps=%lu config=%lu other=%lu encrypted=%lu lastPort=%lu\n",
    (unsigned long)textPackets,
    (unsigned long)telemetryPackets,
    (unsigned long)positionPackets,
    (unsigned long)nodeInfoPackets,
    (unsigned long)remotePositionPackets,
    (unsigned long)configFrames,
    (unsigned long)otherFrames,
    (unsigned long)encryptedPackets,
    (unsigned long)lastPortNum
  );

  if (millis() - lastSdDiagMs >= 5000) {
    lastSdDiagMs = millis();
    refreshSdUsage();
    Serial.printf(
      "[SD] available=%s status=\"%s\" type=%s card=%llu total=%llu used=%llu writes=%lu errors=%lu busy=%lu append=%lu/%lu wait=%u drop=%lu fallback=%lu trunc=%lu\n",
      sdStorage.available ? "true" : "false",
      sdStorage.status,
      sdStorage.cardType,
      (unsigned long long)sdStorage.cardSizeBytes,
      (unsigned long long)sdStorage.totalBytes,
      (unsigned long long)sdStorage.usedBytes,
      (unsigned long)sdStorage.writes,
      (unsigned long)sdStorage.writeErrors,
      (unsigned long)sdLockTimeouts,
      (unsigned long)sdAppendWritten,
      (unsigned long)sdAppendQueued,
      (unsigned)sdAppendQueueWaiting(),
      (unsigned long)sdAppendDropped,
      (unsigned long)sdAppendFallbacks,
      (unsigned long)sdAppendTruncated
    );
    Serial.printf(
      "[TOUCH] samples=%lu last=%s x=%u y=%u age=%lu\n",
      (unsigned long)touchSamples,
      lastTouchMs ? "yes" : "no",
      (unsigned)lastTouchX,
      (unsigned)lastTouchY,
      lastTouchMs ? (unsigned long)((millis() - lastTouchMs) / 1000) : 0
    );
    Serial.printf(
      "[MEM] internal=%lu/%luKB psram=%lu/%luKB heapMin=%luKB\n",
      heapCapKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
      heapCapLargestKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
      heapCapKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
      heapCapLargestKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
      (unsigned long)(ESP.getMinFreeHeap() / 1024)
    );
    Serial.printf(
      "[MAP] render last/max=%lu/%lums count=%lu draws=%lu cache=%lu worker=%lu/%lu/%lu busy=%lu status=\"%s\"\n",
      (unsigned long)lastMapRenderDurationMs,
      (unsigned long)maxMapRenderDurationMs,
      (unsigned long)mapRenderCount,
      (unsigned long)mapRenderDraws,
      (unsigned long)mapRenderCacheHits,
      (unsigned long)mapRenderWorkerStarts,
      (unsigned long)mapRenderWorkerApplies,
      (unsigned long)mapRenderWorkerFailures,
      (unsigned long)mapRenderWorkerBusySkips,
      mapCacheStatus
    );
    Serial.printf("[TILECACHE] slots=%u resident=%lu hit=%lu miss=%lu loads=%lu evict=%lu errors=%lu sdReads=%lu bytes=%lu readUs=%lu\n",
                  mapTileCacheBuffer ? (unsigned)MAP_TILE_CACHE_SLOTS : 0,
                  (unsigned long)mapTileCacheResident.load(), (unsigned long)mapTileCacheHits.load(),
                  (unsigned long)mapTileCacheMisses.load(), (unsigned long)mapTileCacheLoads.load(),
                  (unsigned long)mapTileCacheEvictions.load(), (unsigned long)mapTileCacheReadErrors.load(),
                  (unsigned long)mapTileSdReadCalls.load(), (unsigned long)mapTileSdReadBytes.load(),
                  (unsigned long)lastMapTileReadUs);
  }
}

#include "HomeUiTests.inc"

static void serviceUsbSerialCommands() {
#ifdef FIRMWARE_RUN_TESTS
  if (testScreenPixels && millis() - testScreenCapturedMs > 30000) { heap_caps_free(testScreenPixels); testScreenPixels = nullptr; }
#endif
  static char command[4224];
  static size_t commandLen = 0;
  static bool overflow = false;
  serviceUsbTileTimeout();
  serviceUsbSdTimeout();
  // Bound work per loop so GPS, radio, touch and HTTP keep being serviced.
  size_t budget = 8192;
  while (budget-- && Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflow) {
        Serial.println("[TILEUSB] 00000000 ERROR line-too-long");
        overflow = false;
        commandLen = 0;
        continue;
      }
      command[commandLen] = '\0';
      if (handleUsbTileCommand(command) || handleUsbSdCommand(command)) {
        commandLen = 0;
        continue;
      }
#ifdef FIRMWARE_RUN_TESTS
      if (handleScreenCaptureCommand(command)) {
      } else if (strcmp(command, "test-home") == 0) {
        runHomeUiTests();
      } else if (strcmp(command, "test-webmap") == 0) {
        runWebMapTests();
      } else if (strcmp(command, "test-coverage") == 0) {
        runMapCoverageTests();
      } else if (strcmp(command, "test-map") == 0) {
        runMapNavigationTests();
      } else if (strcmp(command, "test-ui") == 0) {
        runUiSmokeTests();
      } else
#endif
      if (handleGpsControlCommand(command)) {
      } else if (strcmp(command, "gps") == 0) {
        printLocalGpsDiagnostics();
      } else if (strcmp(command, "mapbench") == 0) {
        requestMapBenchmark();
      } else if (strcmp(command, "version") == 0) {
        printFirmwareIdentity();
      } else if (strcmp(command, "diagnostics") == 0) {
        Serial.println(buildFieldReportJson());
      } else if (strcmp(command, "audio") == 0 || strcmp(command, "tone-status") == 0) {
        Serial.println(String("[audio] ") + buildToneStatusJson());
      } else if (strcmp(command, "audio-recover") == 0) {
        requestToneRecovery(true);
        Serial.println("[audio] recovery requested");
      } else if (strcmp(command, "audio-test") == 0) {
        bool accepted = enqueueTone(TONE_TEST, TONE_STYLE_RAW, true, 25);
        Serial.println(accepted ? "[audio] short sine queued (25% maximum)" : "[audio] test unavailable: busy, muted or recovering");
      } else if (strcmp(command, "tiles") == 0 || strcmp(command, "maps") == 0) {
        printSdTileSummary();
      } else if (strcmp(command, "tilesfast") == 0) {
        printSdTileFastSummary();
      } else if (strcmp(command, "tileroots") == 0) {
        detectMapTileRoot(true);
        printMapTileRootCandidates();
      } else if (strcmp(command, "sd") == 0) {
        Serial.printf("[sd] available=%s status=%s type=%s total=%llu used=%llu tileRoot=%s tileRootFound=%s chat=%u/%u/%u append=%lu/%lu wait=%u drop=%lu\n",
                      sdStorage.available ? "true" : "false",
                      sdStorage.status,
                      sdStorage.cardType,
                      (unsigned long long)sdStorage.totalBytes,
                      (unsigned long long)sdStorage.usedBytes,
                      mapTileRoot,
                      mapTileRootFound ? "true" : "false",
                      (unsigned)strlen(publicChatLog),
                      (unsigned)strlen(familyChatLog),
                      (unsigned)strlen(directChatLog),
                      (unsigned long)sdAppendWritten,
                      (unsigned long)sdAppendQueued,
                      (unsigned)sdAppendQueueWaiting(),
                      (unsigned long)sdAppendDropped);
      } else if (commandLen > 0) {
        Serial.printf("[serial] unknown command: %s\n", command);
        Serial.println("[serial] commands: version, diagnostics, audio, audio-test, audio-recover, mapbench, sd, tiles, tilesfast, tileroots, tile-hello");
      }
      commandLen = 0;
      command[0] = '\0';
    } else if (commandLen + 1 < sizeof(command)) {
      command[commandLen++] = c;
    } else overflow = true;
  }
}

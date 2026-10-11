// Internal implementation module; included once by the main sketch.
static const char* linkHealthText() {
  if (!lastByteMs) return "No LoRa data";
  uint32_t age = (millis() - lastByteMs) / 1000;
  if (age > 45) return "No recent RX";
  if (decodeErrors > 0 && decodeErrors > framesDecoded / 2) return "Weak signal";
  return age > 15 ? "RX stale" : "RX active";
}

static const char* shortLinkHealthText() {
  if (!lastByteMs) return "LoRa idle";
  uint32_t age = (millis() - lastByteMs) / 1000;
  if (age > 45) return "LoRa no RX";
  if (decodeErrors > 0 && decodeErrors > framesDecoded / 2) return "LoRa weak";
  return age > 15 ? "LoRa stale" : "LoRa OK";
}

static uint32_t loraStatusColor() {
  if (!lastByteMs) return 0xFF5A5F;
  uint32_t age = (millis() - lastByteMs) / 1000;
  if (age > 45) return 0xFF5A5F;
  if (decodeErrors > 0 && decodeErrors > framesDecoded / 2) return 0xFFD166;
  if (age > 15) return 0xFFD166;
  return COLOR_ACTION;
}

static bool gpsSatsPresent() {
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
    snprintf(line, sizeof(line), "LoRa\n%s", linkHealthText());
    lv_label_set_text(lblHomeLink, line);
  }
  if (lblHomeGps) {
    snprintf(line, sizeof(line), "GPS\n%s", gpsSatsPresent() ? "Sats" : (gpsStats.valid ? "Fix" : "Waiting"));
    lv_label_set_text(lblHomeGps, line);
  }
  if (lblHomeMessages) {
    uint16_t unreadTotal = unreadPublic + unreadFamily + unreadDirect;
    const char* latest = previewFamily[0] ? previewFamily : (previewPublic[0] ? previewPublic : (previewDirect[0] ? previewDirect : "No messages yet"));
    snprintf(line, sizeof(line), "Messages %u new\n%s", unreadTotal, latest);
    lv_label_set_text(lblHomeMessages, line);
  }
  if (lblHomePower) {
    snprintf(line, sizeof(line), "Power %d%% %s\n%s %s",
             localBattery.percent,
             localBattery.charging ? "charging" : "battery",
             privateChannelLabel(),
             privateChannelIndex >= 0 ? "ready" : "not loaded");
    lv_label_set_text(lblHomePower, line);
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

static void refreshMapUi() {
  if (!mapPlot || !lblMapStats) return;
  if (currentPage != pageGps) return;
  uint32_t minIntervalMs = mapRenderPending ? 750 : 5000;
  if (lastMapUiRefreshMs && millis() - lastMapUiRefreshMs < minIntervalMs) return;
  lastMapUiRefreshMs = millis();
  mapRenderPending = false;

  const NodeRecord* selected = mapCenterMode == MAP_CENTER_SELECTED_NODE ? findNode(selectedNodeNum) : nullptr;
  const NodeRecord* ownNode = stats.myNodeNum ? findNode(stats.myNodeNum) : nullptr;
  const bool hasLocalFix = gpsStats.valid;
  const bool hasOwnNodePosition = ownNode && ownNode->hasPosition;
  const bool focusSelected = mapCenterMode == MAP_CENTER_SELECTED_NODE && selected && selected->hasPosition;
  const bool preferLastLocation = mapCenterMode == MAP_CENTER_LAST_LOCATION;
  const bool hasSavedCenter = lastMapLocationValid;
  const bool hasMapCenter = focusSelected || (!preferLastLocation && hasLocalFix) || (!preferLastLocation && hasOwnNodePosition) || hasSavedCenter;
  double centerLat = focusSelected ? selected->latitude : ((!preferLastLocation && hasLocalFix) ? gpsStats.latitude : ((!preferLastLocation && hasOwnNodePosition) ? ownNode->latitude : defaultMapLat));
  double centerLon = focusSelected ? selected->longitude : ((!preferLastLocation && hasLocalFix) ? gpsStats.longitude : ((!preferLastLocation && hasOwnNodePosition) ? ownNode->longitude : defaultMapLon));
  const char* centerSource = focusSelected ? "Selected node" : ((!preferLastLocation && hasLocalFix) ? "CYD GPS" : ((!preferLastLocation && hasOwnNodePosition) ? "This device" : "Last device location"));
  for (size_t i = 0; i < MAP_DOT_COUNT; i++) {
    if (mapDots[i]) lv_obj_add_flag(mapDots[i], LV_OBJ_FLAG_HIDDEN);
  }

  if (!hasMapCenter) {
    if (mapCanvas) {
      lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
      lv_obj_invalidate(mapCanvas);
    }
    mapCanvasCached = false;
    char waitingText[160];
    snprintf(waitingText, sizeof(waitingText),
             "%s\n"
             "Cache: %s\n"
             "RX GPIO%d: %lu bytes\nSD maps: %s",
             mapFocusSelectedNode ? "Selected node has no GPS fix..." : "Waiting for current device location...",
             mapCacheStatus,
             GPS_RX_PIN,
             (unsigned long)gpsBytesFromLocal,
             sdStorage.available ? "ready" : sdStorage.status);
    lv_label_set_text(lblMapStats, waitingText);
    clearUiDirty(UI_DIRTY_MAP);
    return;
  }

  if (!sdStorage.available || !mapTileRootFound) {
    char waitingText[180];
    snprintf(waitingText, sizeof(waitingText),
             "%s\n"
             "Center: %.5f, %.5f\n"
             "%s\n"
             "Tile root: %s",
             mapCanvasCached ? "Showing cached map" : "Map tiles unavailable",
             centerLat,
             centerLon,
             sdStorage.available ? "No tile folder found" : sdStorage.status,
             mapTileRoot);
    lv_label_set_text(lblMapStats, waitingText);
    clearUiDirty(UI_DIRTY_MAP);
    return;
  }

  char tilePath[96];
  int mapZoom = cachedMapZoom;
  const uint16_t plotH = (uint16_t)activeMapPlotHeight();
  bool usingCachedZoom = mapZoom >= MAP_TILE_MIN_ZOOM &&
                         mapZoom <= MAP_TILE_MAX_ZOOM &&
                         mapCacheMatchesCenter(centerLat, centerLon, mapZoom, plotH);
  if (!usingCachedZoom) {
    mapZoom = findBestMapZoom(centerLat, centerLon, tilePath, sizeof(tilePath));
    if (mapZoomedOut) mapZoom = max(MAP_TILE_MIN_ZOOM, mapZoom - 2);
  } else {
    buildTilePath(mapTileRoot, mapZoom, cachedMapTileX, cachedMapTileY, tilePath, sizeof(tilePath));
  }
  bool centerTileFound = renderOfflineTileMap(centerLat, centerLon, mapZoom, tilePath, sizeof(tilePath));
  size_t plotted = 0;
  const NodeRecord* nearest = nullptr;
  bool selectedPlotted = false;
  double nearestMeters = 0.0;
  for (size_t i = 0; i < nodeCount; i++) {
    if (!nodes[i].hasPosition) continue;
    bool isSelected = selected && nodes[i].num == selected->num;
    lv_color_t color = nodes[i].num == stats.myNodeNum ? lv_color_hex(0x00C985) : lv_color_hex(isSelected ? 0xFFD166 : 0x68FFC0);
    int dotSize = isSelected ? 14 : 8;
    if (placeMapDot(i, centerLat, centerLon, nodes[i].latitude, nodes[i].longitude, mapZoom, color, dotSize)) {
      if (isSelected) {
        selectedPlotted = true;
        lv_obj_set_style_border_width(mapDots[i], 2, 0);
        lv_obj_set_style_border_color(mapDots[i], lv_color_hex(0xFFF3C4), 0);
        lv_obj_move_foreground(mapDots[i]);
      }
      plotted++;
      if (nodes[i].num != stats.myNodeNum) {
        double meters = distanceMeters(centerLat, centerLon, nodes[i].latitude, nodes[i].longitude);
        if (!nearest || meters < nearestMeters) {
          nearest = &nodes[i];
          nearestMeters = meters;
        }
      }
    }
  }

  if (hasLocalFix) {
    placeMapDot(MAP_DOT_COUNT - 1, centerLat, centerLon, gpsStats.latitude, gpsStats.longitude, mapZoom, lv_color_hex(0x00C985), 10);
    plotted++;
  }

  long tileX = lonToTileX(centerLon, mapZoom);
  long tileY = latToTileY(centerLat, mapZoom);

  char nearestText[72];
  if (nearest) snprintf(nearestText, sizeof(nearestText), "%s %.1f km", nodeName(nearest->num), nearestMeters / 1000.0);
  else strlcpy(nearestText, "none", sizeof(nearestText));

  char tileText[48];
  snprintf(tileText, sizeof(tileText), "Tile z%d/%ld/%ld %s", mapZoom, tileX, tileY, centerTileFound ? "drawn" : "missing");

  char mapText[320];
  if (selected) {
    char selectedRange[32];
    formatRangeBearing(*selected, selectedRange, sizeof(selectedRange));
    if (selected->hasPosition) {
      uint32_t posAge = selected->lastPositionMs ? (millis() - selected->lastPositionMs) / 1000 : 0;
      snprintf(mapText, sizeof(mapText),
               "%s%s\n"
               "%s | pos %lu s\n"
               "%s\n"
               "%u point%s | nearest %s",
               nodeName(selected->num),
               focusSelected ? " centered" : (selectedPlotted ? " on map" : " selected"),
               selectedRange,
               (unsigned long)posAge,
               tileText,
               (unsigned)plotted,
               plotted == 1 ? "" : "s",
               nearestText);
    } else {
      snprintf(mapText, sizeof(mapText),
               "%s selected\n"
               "No node GPS yet\n"
               "%s %.5f, %.5f\n"
               "%s",
               nodeName(selected->num),
               centerSource,
               centerLat,
               centerLon,
               tileText);
    }
  } else {
    snprintf(mapText, sizeof(mapText),
             "%s%s\n"
             "Center: %.5f, %.5f\n"
             "%u point%s | nearest %s\n"
             "%s",
             centerSource,
             mapZoomedOut ? " wide" : "",
             centerLat,
             centerLon,
             (unsigned)plotted,
             plotted == 1 ? "" : "s",
             nearestText,
             tileText);
  }
  lv_label_set_text(lblMapStats, mapText);
  clearUiDirty(UI_DIRTY_MAP);
}

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
      unsigned long shownSats = gpsStats.sats > 99 ? 99 : gpsStats.sats;
      snprintf(gpsStatus, sizeof(gpsStatus), LV_SYMBOL_GPS " %lu", shownSats);
    } else {
      strlcpy(gpsStatus, LV_SYMBOL_GPS " 0", sizeof(gpsStatus));
    }
    lv_label_set_text(lblGpsStatus, gpsStatus);
    lv_obj_set_style_text_color(lblGpsStatus, lv_color_hex(satsPresent ? COLOR_ACTION : 0xFF5A5F), 0);
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
             "S3 interface\n"
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
    char batteryText[960];
    uint32_t gaugeAge = localBattery.lastGaugeMs ? (millis() - localBattery.lastGaugeMs) / 1000 : 0;
    if (localBattery.gaugePresent) {
      snprintf(batteryText, sizeof(batteryText),
               "LiPo fuel gauge\n"
               "Chip: MAX17048  v0x%04X\n"
               "Displayed: %d%%\n"
               "Alert: %s\n"
               "Displayed source: %s\n"
               "Gauge SOC: %.1f%%  raw 0x%04X\n"
               "MAX corrected: %d%%  trim %.1f%%\n"
               "Pack filtered: %.3f V\n"
               "Pack raw: %.3f V\n"
               "Code voltage estimate: %d%%\n"
               "Voltage filter: %s  rejected %lu\n"
               "Voltage low/critical: %s/%s\n"
               "Charge rate: %.1f%%/hr\n"
               "Last change: %ld mV\n"
               "Voltage trend: %.1f mV/min\n"
               "Charging: %s\n"
               "Learning samples: %u\n"
               "Quick-start: %s\n"
               "Updated: %lu s ago\n\n"
               "Power path\n"
               "LiPo > charger > booster > 5V pins\n\n"
               "S3 interface\n"
               "Uptime: %lu s\n"
               "Heap free/min: %lu/%lu KB\n"
               "PSRAM free: %lu KB",
               localBattery.gaugeVersion,
               localBattery.percent,
               batteryAlertLabel(),
               localBattery.percentSource,
               localBattery.gaugeSoc,
               localBattery.rawGaugeSoc,
               localBattery.correctedGaugePercent,
               localBattery.calibrationOffsetTenths / 10.0f,
               localBattery.batteryMv / 1000.0f,
               localBattery.rawPackMv / 1000.0f,
               localBattery.voltagePercent,
               localBattery.voltageUnstable ? "settling" : "stable",
               (unsigned long)localBattery.voltageRejectedSamples,
               localBattery.voltageLowSustained ? "yes" : "no",
               localBattery.voltageCriticalSustained ? "yes" : "no",
               localBattery.chargeRateValid ? localBattery.chargeRatePercentHr : 0.0f,
               (long)localBattery.instantDeltaMv,
               localBattery.deltaMvPerMinTenths / 10.0f,
               localBattery.charging ? "yes" : "no",
               localBattery.stableSampleCount,
               localBattery.quickStartSent ? "sent" : "not needed",
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
      snprintf(gpsText, sizeof(gpsText),
               "Waiting for GPS...\n\n"
               "CYD local GPS:\n"
               "RX GPIO%d  %lu bytes\n\n"
               "Heltec GPS can still feed\n"
               "Meshtastic position packets.",
               GPS_RX_PIN,
               (unsigned long)gpsBytesFromLocal);
    }
    lv_label_set_text(lblGpsStats, gpsText);
  }

  refreshMapUi();

  refreshChatViews(pageForce || isUiDirty(UI_DIRTY_CHAT));
  if (currentPage == pagePacketInspector && taPacketInspector && (pageForce || isUiDirty(UI_DIRTY_PACKET_LOG))) {
    setReadonlyTextStable(taPacketInspector, packetLog[0] ? packetLog : "No packets decoded yet");
    clearUiDirty(UI_DIRTY_PACKET_LOG);
  }
  if (currentPage == pageTxHistory && (pageForce || isUiDirty(UI_DIRTY_TX))) {
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
    "[LINK] rx=%lu tx=%lu last=%s frames=%lu decoded=%lu err=%lu magic=%lu/%lu badlen=%lu hb=%lu resync=%lu ascii=\"%.60s\"\n",
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
  }
}

static void serviceUsbSerialCommands() {
  static char command[48];
  static size_t commandLen = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      command[commandLen] = '\0';
      if (strcmp(command, "tiles") == 0 || strcmp(command, "maps") == 0) {
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
        Serial.println("[serial] commands: sd, tiles, tilesfast, tileroots");
      }
      commandLen = 0;
      command[0] = '\0';
    } else if (commandLen + 1 < sizeof(command)) {
      command[commandLen++] = c;
    }
  }
}

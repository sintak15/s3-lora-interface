// Internal implementation module; included once by the main sketch.
static String jsonEscape(const char* text) {
  String out;
  if (!text) return out;
  out.reserve(strlen(text) + 8);
  while (*text) {
    char c = *text++;
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if ((uint8_t)c >= 0x20) {
      out += c;
    }
  }
  return out;
}

static void appendTileCacheStatusJson(String& json) {
  json += "\"mapTileCacheEnabled\":" + String(mapTileCacheBuffer ? "true" : "false") + ",";
  json += "\"mapTileCacheSlots\":" + String(mapTileCacheBuffer ? (unsigned)MAP_TILE_CACHE_SLOTS : 0) + ",";
  json += "\"mapTileCacheKb\":" + String(mapTileCacheBuffer ? (unsigned)(MAP_TILE_CACHE_SLOTS * MAP_TILE_BYTES / 1024) : 0) + ",";
  json += "\"mapTileCacheResident\":" + String(mapTileCacheResident.load()) + ",";
  json += "\"mapTileCacheHits\":" + String(mapTileCacheHits.load()) + ",";
  json += "\"mapTileCacheMisses\":" + String(mapTileCacheMisses.load()) + ",";
  json += "\"mapTileCacheLoads\":" + String(mapTileCacheLoads.load()) + ",";
  json += "\"mapTileCacheEvictions\":" + String(mapTileCacheEvictions.load()) + ",";
  json += "\"mapTileReadErrors\":" + String(mapTileCacheReadErrors.load()) + ",";
  json += "\"mapTileSdReadCalls\":" + String(mapTileSdReadCalls.load()) + ",";
  json += "\"mapTileSdReadBytes\":" + String(mapTileSdReadBytes.load()) + ",";
  json += "\"mapTileLastReadUs\":" + String(lastMapTileReadUs) + ",";
  json += "\"mapTileStorageGeneration\":" + String(mapStorageGeneration.load()) + ",";
  bool coverageChecked = mapCoverage.ready && mapCoverage.generation == mapStorageGeneration.load() && sdStorage.available && mapTileRootFound;
  json += "\"mapRequestedZoom\":" + String(mapView.zoom) + ",";
  json += "\"mapDisplayedZoom\":" + (mapCanvasCached ? String(cachedMapZoom) : String("null")) + ",";
  json += "\"mapCoverageChecked\":" + String(coverageChecked ? "true" : "false") + ",";
  json += "\"mapCoverageAgeSec\":" + (coverageChecked ? String((millis() - mapCoverage.checkedMs) / 1000) : String("null")) + ",";
  json += "\"mapCoverageLevels\":[";
  for (int i = 0; i < MapCoverage::levels; ++i) {
    if (i) json += ",";
    const auto& level = mapCoverage.zooms[i];
    json += "{\"zoom\":" + String(i + MAP_TILE_MIN_ZOOM) + ",\"status\":\"" + String(coverageChecked ? MapCoverage::status(level) : "Unchecked") + "\",";
    json += "\"available\":" + String(coverageChecked ? level.available() : 0) + ",\"required\":" + String(coverageChecked ? (unsigned)level.area.count : 0) + ",";
    json += "\"invalid\":" + String(coverageChecked ? level.invalid() : 0) + ",\"center\":" + String(coverageChecked && level.center() ? "true" : "false") + "}";
  }
  json += "],";
  json += "\"mapBenchmarkRunning\":" + String(mapBenchmarkRunning ? "true" : "false") + ",";
  json += "\"mapBenchmark\":";
  if (!lastMapBenchmarkAvailable) json += "null,";
  else {
    json += "{\"passed\":" + String(lastMapBenchmarkPassed ? "true" : "false") + ",\"samples\":[";
    static const char* labels[] = {"baseline", "cold", "warm", "baseline-pan", "cached-pan", "baseline-return", "cached-return"};
    for (size_t i = 0; i < 7; ++i) {
      if (i) json += ",";
      const auto& sample = lastMapBenchmark[i];
      json += "{\"mode\":\"" + String(labels[i]) + "\",\"durationUs\":" + String(sample.durationUs) + ",";
      json += "\"readCalls\":" + String(sample.readCalls) + ",\"readBytes\":" + String(sample.readBytes) + ",";
      json += "\"ok\":" + String(sample.ok ? "true" : "false") + ",\"pixelsMatch\":" + String(sample.pixelsMatch ? "true" : "false") + "}";
    }
    json += "]},";
  }
}

static void appendBatteryStatusJson(String& json) {
  json += "\"battery\":" + String(localBattery.percent) + ",";
  json += "\"voltage\":" + String(localBattery.batteryMv / 1000.0f, 2) + ",";
  json += "\"fuelGauge\":\"" + String(localBattery.gaugePresent ? "MAX17048" : "none") + "\",";
  json += "\"fuelGaugePresent\":" + String(localBattery.gaugePresent ? "true" : "false") + ",";
  json += "\"fuelGaugeSoc\":" + String(localBattery.gaugeSoc, 1) + ",";
  // Legacy correction fields now mirror the gauge; no software SOC trim applies.
  json += "\"fuelGaugeCorrectedSoc\":" + String(localBattery.gaugeSoc, 1) + ",";
  json += "\"fuelGaugeCorrectedPercent\":" + String(localBattery.gaugePercent) + ",";
  json += "\"fuelGaugeSocReliable\":" + String(localBattery.gaugeSocReliable ? "true" : "false") + ",";
  json += "\"fuelGaugeRawSoc\":\"0x" + String(localBattery.rawGaugeSoc, HEX) + "\",";
  json += "\"fuelGaugeQuickStartSent\":false,";
  json += "\"fuelGaugeVersion\":\"0x" + String(localBattery.gaugeVersion, HEX) + "\",";
  json += "\"senseVoltage\":" + String(localBattery.filteredPackMv / 1000.0f, 2) + ",";
  json += "\"rawSenseVoltage\":" + String(localBattery.rawPackMv / 1000.0f, 2) + ",";
  json += "\"batteryVoltageUnstable\":" + String(localBattery.voltageUnstable ? "true" : "false") + ",";
  json += "\"batteryVoltageRejectedSamples\":" + String(localBattery.voltageRejectedSamples) + ",";
  json += "\"batteryVoltageStableSamples\":" + String(localBattery.voltageStableSamples) + ",";
  json += "\"batterySampleCount\":" + String(localBattery.sampleCount) + ",";
  json += "\"batteryLow\":" + String(localBattery.batteryLow ? "true" : "false") + ",";
  json += "\"batteryCritical\":" + String(localBattery.batteryCritical ? "true" : "false") + ",";
  json += "\"batteryHealth\":\"" + String(batteryAlertLabel()) + "\",";
  json += "\"batteryVoltageLowSustained\":" + String(localBattery.voltageLowSustained ? "true" : "false") + ",";
  json += "\"batteryVoltageCriticalSustained\":" + String(localBattery.voltageCriticalSustained ? "true" : "false") + ",";
  json += "\"batterySource\":\"" + jsonEscape(localBattery.percentSource) + "\",";
  json += "\"powerState\":\"" + jsonEscape(localBattery.powerState) + "\",";
  json += "\"batteryTrend\":" + String(localBattery.deltaMvPerMinTenths / 10.0f, 1) + ",";
  json += "\"batteryTrendMvPerMin\":" + String(localBattery.deltaMvPerMin) + ",";
  json += "\"batteryChargeRatePercentHr\":" + String(localBattery.chargeRatePercentHr, 1) + ",";
  json += "\"batteryChargeRateValid\":" + String(localBattery.chargeRateValid ? "true" : "false") + ",";
  json += "\"batteryCharging\":" + String(localBattery.charging ? "true" : "false") + ",";
  uint32_t trendWindowSec = 0;
  if (localBattery.trendCount >= 2) {
    uint8_t trendWindow = sizeof(localBattery.trendMv) / sizeof(localBattery.trendMv[0]);
    uint8_t newestIndex = (localBattery.trendHead + trendWindow - 1) % trendWindow;
    uint8_t oldestIndex = (localBattery.trendHead + trendWindow - localBattery.trendCount) % trendWindow;
    if (localBattery.trendMs[newestIndex] > localBattery.trendMs[oldestIndex]) {
      trendWindowSec = (localBattery.trendMs[newestIndex] - localBattery.trendMs[oldestIndex]) / 1000;
    }
  }
  json += "\"batteryRuntimeMinutes\":" + String(batteryRuntime.minutes()) + ",";
  json += "\"batteryRuntimeBasis\":\"" + String(batteryRuntime.basis()) + "\",";
  json += "\"batteryLearnedDischargePercentHr\":" + String(batteryRuntime.learnedPercentPerHour(), 2) + ",";
  json += "\"batterySavedDischargePercentHr\":" + String(localBattery.savedDischargeRate, 2) + ",";
  json += "\"batteryDischargePercentHr\":" + String(batteryRuntime.percentPerHour(), 2) + ",";
  json += "\"batteryTrendWindowSec\":" + String(trendWindowSec) + ",";
  json += "\"batteryCalibrationOffsetPercent\":0,";
  json += "\"batteryCalibrationOffsetMv\":0,";
  json += "\"batteryLearnedVoltage\":" + String(localBattery.learnedBatteryMv / 1000.0f, 2) + ",";
}

static void appendSdStatusJson(String& json) {
  json += "\"sdAvailable\":" + String(sdStorage.available ? "true" : "false") + ",";
  json += "\"sdStatus\":\"" + jsonEscape(sdStorage.status) + "\",";
  json += "\"sdType\":\"" + jsonEscape(sdStorage.cardType) + "\",";
  json += "\"sdUsedKb\":" + String(bytesToWholeKb(sdStorage.usedBytes)) + ",";
  json += "\"sdTotalKb\":" + String(bytesToWholeKb(sdStorage.totalBytes)) + ",";
  json += "\"sdUsedMb\":" + String(bytesToWholeMb(sdStorage.usedBytes)) + ",";
  json += "\"sdSizeMb\":" + String(bytesToWholeMb(sdStorage.totalBytes)) + ",";
  json += "\"sdWrites\":" + String(sdStorage.writes) + ",";
  json += "\"sdErrors\":" + String(sdStorage.writeErrors) + ",";
  json += "\"sdLockTimeouts\":" + String(sdLockTimeouts) + ",";
  json += "\"sdAppendQueueDepth\":" + String((unsigned)SD_APPEND_QUEUE_DEPTH) + ",";
  json += "\"sdAppendQueueWaiting\":" + String((unsigned)sdAppendQueueWaiting()) + ",";
  json += "\"sdAppendQueued\":" + String(sdAppendQueued) + ",";
  json += "\"sdAppendWritten\":" + String(sdAppendWritten) + ",";
  json += "\"sdAppendFallbacks\":" + String(sdAppendFallbacks) + ",";
  json += "\"sdAppendDropped\":" + String(sdAppendDropped) + ",";
  json += "\"sdAppendTruncated\":" + String(sdAppendTruncated) + ",";
}

static void appendTxStatusJson(String& json) {
  json += "\"txHistory\":[";
  for (size_t offset = 0; offset < txHistoryCount; ++offset) {
    if (offset) json += ",";
    const TxRecord& record = txHistory[(txHistoryNext + TX_HISTORY_COUNT - 1 - offset) % TX_HISTORY_COUNT];
    char destination[24];
    describeTxDestination(record, destination, sizeof(destination));
    json += "{\"id\":" + String(record.packetId) + ",\"destination\":\"" + jsonEscape(destination) + "\",";
    json += "\"text\":\"" + jsonEscape(record.text) + "\",\"status\":\"" + txStatusText(record, millis()) + "\",";
    json += "\"ageSec\":" + String((millis() - record.sentMs) / 1000) + ",";
    json += "\"error\":\"" + String(txErrorText(record)) + "\",\"errorCode\":" + String(record.errorCode) + ",";
    json += "\"localEcho\":" + String(record.echoSeen ? "true" : "false") + ",";
    json += "\"retriedAs\":" + String(record.retriedAs) + ",";
    json += "\"canRetry\":" + String(!record.retriedAs && FieldStatus::retryAllowed(record.delivery, record.sentMs, millis()) ? "true" : "false") + "}";
  }
  json += "],";
}

static void appendChatStatusJson(String& json) {
  json += "\"chat\":\"" + jsonEscape(publicChatLog) + "\",";
  json += "\"publicChat\":\"" + jsonEscape(publicChatLog) + "\",";
  json += "\"secondaryChat\":\"" + jsonEscape(familyChatLog) + "\",";
  json += "\"privateChat\":\"" + jsonEscape(familyChatLog) + "\",";
  json += "\"familyChat\":\"" + jsonEscape(familyChatLog) + "\",";
  json += "\"directChat\":\"" + jsonEscape(directChatLog) + "\",";
  json += "\"chatHistoryLoaded\":" + String(chatHistoryLoadedFromSd ? "true" : "false") + ",";
  json += "\"publicChatBytes\":" + String(strlen(publicChatLog)) + ",";
  json += "\"secondaryChatBytes\":" + String(strlen(familyChatLog)) + ",";
  json += "\"privateChatBytes\":" + String(strlen(familyChatLog)) + ",";
  json += "\"directChatBytes\":" + String(strlen(directChatLog)) + ",";
  json += "\"secondaryChannel\":" + String(privateChannelIndex) + ",";
  json += "\"privateChannel\":" + String(privateChannelIndex) + ",";
}

static void appendLinkStatusJson(String& json, const char* rxAge) {
  json += "\"linkHealth\":\"" + String(linkHealthText()) + "\",";
  json += "\"lastRadioResponseSec\":" + (radioResponseSeen ? String((millis() - lastRadioResponseMs) / 1000) : String("null")) + ",";
  json += "\"frames\":" + String(framesDecoded) + ",";
  json += "\"errors\":" + String(decodeErrors) + ",";
  json += "\"bytes\":" + String(bytesFromRadio) + ",";
  json += "\"txBytes\":" + String(bytesToRadio) + ",";
  json += "\"lastByte\":\"" + jsonEscape(rxAge) + "\",";
  json += "\"heartbeats\":" + String(heartbeatCount) + ",";
  json += "\"apiResyncs\":" + String(apiResyncCount) + ",";
  json += "\"frameTimeouts\":" + String(serialFrameTimeouts) + ",";
  json += "\"nextPacketId\":" + String(nextMeshPacketId) + ",";
  json += "\"magic1\":" + String(magic1Count) + ",";
  json += "\"magic2\":" + String(magic2Count) + ",";
  json += "\"streamFrames\":" + String(streamFrames) + ",";
  json += "\"badLengths\":" + String(invalidFrameLengths) + ",";
  json += "\"textPackets\":" + String(textPackets) + ",";
  json += "\"telemetryPackets\":" + String(telemetryPackets) + ",";
  json += "\"positionPackets\":" + String(positionPackets) + ",";
  json += "\"remotePositionPackets\":" + String(remotePositionPackets) + ",";
  json += "\"nodeInfoPackets\":" + String(nodeInfoPackets) + ",";
  json += "\"configFrames\":" + String(configFrames) + ",";
  json += "\"otherFrames\":" + String(otherFrames) + ",";
  json += "\"encryptedPackets\":" + String(encryptedPackets) + ",";
  json += "\"lastPort\":" + String(lastPortNum) + ",";
  json += "\"serialPeek\":\"" + jsonEscape(serialPeek) + "\",";
}

static void appendInterfaceStatusJson(String& json, const String& wifiIp, const esp_partition_t* runningPartition) {
  json += "\"title\":\"" + jsonEscape(interfaceDeviceName) + "\",";
  json += "\"deviceName\":\"" + jsonEscape(interfaceDeviceName) + "\",";
  json += "\"hostname\":\"" + jsonEscape(interfaceHostname) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(interfaceApSsid) + "\",";
  json += "\"apChannel\":" + String(interfaceApChannel) + ",";
  json += "\"webUser\":\"" + jsonEscape(webUiUser) + "\",";
  json += "\"setupRequired\":" + String(usingDefaultWebCredentials() ? "true" : "false") + ",";
  json += "\"firmwareVersion\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"gitCommit\":\"" + String(FIRMWARE_GIT_COMMIT) + "\",";
  json += "\"sourceDirty\":" + String(FIRMWARE_SOURCE_DIRTY ? "true" : "false") + ",";
  json += "\"buildDate\":\"" + String(__DATE__) + "\",";
  json += "\"buildTime\":\"" + String(__TIME__) + "\",";
  json += "\"runningPartition\":\"" + String(runningPartition ? runningPartition->label : "unknown") + "\",";
  json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
  json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"ip\":\"" + wifiIp + "\",";
  json += "\"wifiEnabled\":" + String(wifiEnabled ? "true" : "false") + ",";
  json += "\"wifiMode\":\"" + String(wifiApMode ? "AP" : "Local") + "\",";
  json += "\"wifiStations\":" + String((wifiEnabled && wifiApMode) ? WiFi.softAPgetStationNum() : 0) + ",";
  json += "\"wifiToggles\":" + String(wifiToggleCount) + ",";
  json += "\"toneVolume\":" + String(toneVolumePercent) + ",";
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
  json += "\"toneLastMs\":" + String(lastToneMs) + ",";
  json += "\"tonePending\":" + String((int)pendingToneKind) + ",";
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
  json += "\"toneLastI2sError\":" + String(toneLastI2sError) + ",";
  json += "\"toneDataPin\":" + String(CYD_AUDIO_DOUT_PIN) + ",";
  json += "\"toneRecoveries\":" + String(toneRecoveries) + ",";
  json += "\"toneDropped\":" + String(toneWorker.dropped.load()) + ",";
  json += "\"toneRecovering\":" + String(toneWorker.pauseRequested.load() ? "true" : "false") + ",";
}

static void appendMapStatusJson(String& json) {
  json += "\"offlineMapCenter\":" + (lastMapLocationValid ? String("{\"lat\":") + String(defaultMapLat, 6) + ",\"lon\":" + String(defaultMapLon, 6) + "}" : String("null")) + ",";
  json += "\"gpsValid\":" + String(gpsStats.valid ? "true" : "false") + ",";
  json += "\"gpsLat\":" + String(gpsStats.latitude, 6) + ",";
  json += "\"gpsLon\":" + String(gpsStats.longitude, 6) + ",";
  appendLocalGpsHealthJson(json);
  json += "\"positionedNodes\":" + String(countPositionedNodes()) + ",";
  json += "\"mapCacheStatus\":\"" + jsonEscape(mapCacheStatus) + "\",";
  json += "\"mapCacheLoaded\":" + String(mapCanvasCached ? "true" : "false") + ",";
  json += "\"mapTileRoot\":\"" + jsonEscape(mapTileRoot) + "\",";
  json += "\"mapTileRootFound\":" + String(mapTileRootFound ? "true" : "false") + ",";
  json += "\"heapFreeKb\":" + String(ESP.getFreeHeap() / 1024) + ",";
  json += "\"heapMinKb\":" + String(ESP.getMinFreeHeap() / 1024) + ",";
  json += "\"internalFreeKb\":" + String(heapCapKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + ",";
  json += "\"internalLargestKb\":" + String(heapCapLargestKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + ",";
  json += "\"psramFreeKb\":" + String(heapCapKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) + ",";
  json += "\"psramLargestKb\":" + String(heapCapLargestKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) + ",";
  json += "\"mapRenderLastMs\":" + String(lastMapRenderDurationMs) + ",";
  json += "\"mapRenderMaxMs\":" + String(maxMapRenderDurationMs) + ",";
  json += "\"mapRenderFinishedMs\":" + String(lastMapRenderFinishedMs) + ",";
  json += "\"mapRenderCount\":" + String(mapRenderCount) + ",";
  json += "\"mapRenderDraws\":" + String(mapRenderDraws) + ",";
  json += "\"mapRenderCacheHits\":" + String(mapRenderCacheHits) + ",";
  json += "\"mapRenderWorkerRunning\":" + String(mapRenderWorkerRunning ? "true" : "false") + ",";
  json += "\"mapRenderResultPending\":" + String(mapRenderResultPending ? "true" : "false") + ",";
  json += "\"mapRenderWorkerStarts\":" + String(mapRenderWorkerStarts) + ",";
  json += "\"mapRenderWorkerApplies\":" + String(mapRenderWorkerApplies) + ",";
  json += "\"mapRenderWorkerFailures\":" + String(mapRenderWorkerFailures) + ",";
  json += "\"mapRenderWorkerBusySkips\":" + String(mapRenderWorkerBusySkips) + ",";
}

static void appendHeltecConfigStatusJson(String& json) {
  json += "\"heltecConfig\":{";
  json += "\"ageSec\":" + String(heltecConfig.lastConfigMs ? (millis() - heltecConfig.lastConfigMs) / 1000 : -1) + ",";
  json += "\"moduleAgeSec\":" + String(heltecConfig.lastModuleMs ? (millis() - heltecConfig.lastModuleMs) / 1000 : -1) + ",";
  json += "\"lora\":{\"valid\":" + String(heltecConfig.hasLora ? "true" : "false") + ",";
  json += "\"region\":\"" + String(loraRegionName(heltecConfig.lora.region)) + "\",";
  json += "\"preset\":\"" + String(loraPresetName(heltecConfig.lora.modem_preset)) + "\",";
  json += "\"hop\":" + String(heltecConfig.lora.hop_limit) + ",";
  json += "\"txPower\":" + String(heltecConfig.lora.tx_power) + ",";
  json += "\"txEnabled\":" + String(heltecConfig.lora.tx_enabled ? "true" : "false") + ",";
  json += "\"channelNum\":" + String(heltecConfig.lora.channel_num) + "},";
  json += "\"serial\":{\"valid\":" + String(heltecConfig.hasSerial ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.serial.enabled ? "true" : "false") + ",";
  json += "\"echo\":" + String(heltecConfig.serial.echo ? "true" : "false") + ",";
  json += "\"rxd\":" + String(heltecConfig.serial.rxd) + ",";
  json += "\"txd\":" + String(heltecConfig.serial.txd) + ",";
  json += "\"baud\":\"" + String(serialBaudName(heltecConfig.serial.baud)) + "\",";
  json += "\"mode\":\"" + String(serialModeName(heltecConfig.serial.mode)) + "\",";
  json += "\"timeout\":" + String(heltecConfig.serial.timeout) + ",";
  json += "\"override\":" + String(heltecConfig.serial.override_console_serial_port ? "true" : "false") + "},";
  json += "\"device\":{\"valid\":" + String(heltecConfig.hasDevice ? "true" : "false") + ",";
  json += "\"role\":\"" + String(deviceRoleName(heltecConfig.device.role)) + "\",";
  json += "\"rebroadcast\":\"" + String(rebroadcastName(heltecConfig.device.rebroadcast_mode)) + "\",";
  json += "\"nodeInfo\":" + String(heltecConfig.device.node_info_broadcast_secs) + ",";
  json += "\"tz\":\"" + jsonEscape(heltecConfig.device.tzdef) + "\",";
  json += "\"ledOff\":" + String(heltecConfig.device.led_heartbeat_disabled ? "true" : "false") + ",";
  json += "\"buzzer\":\"" + String(buzzerName(heltecConfig.device.buzzer_mode)) + "\"},";
  json += "\"position\":{\"valid\":" + String(heltecConfig.hasPosition ? "true" : "false") + ",";
  json += "\"gpsEnabled\":" + String(heltecConfig.position.gps_enabled ? "true" : "false") + ",";
  json += "\"gpsMode\":\"" + String(gpsModeName(heltecConfig.position.gps_mode)) + "\",";
  json += "\"fixed\":" + String(heltecConfig.position.fixed_position ? "true" : "false") + ",";
  json += "\"smart\":" + String(heltecConfig.position.position_broadcast_smart_enabled ? "true" : "false") + ",";
  json += "\"broadcast\":" + String(heltecConfig.position.position_broadcast_secs) + ",";
  json += "\"gpsUpdate\":" + String(heltecConfig.position.gps_update_interval) + ",";
  json += "\"gpsAttempt\":" + String(heltecConfig.position.gps_attempt_time) + ",";
  json += "\"smartMeters\":" + String(heltecConfig.position.broadcast_smart_minimum_distance) + ",";
  json += "\"smartSecs\":" + String(heltecConfig.position.broadcast_smart_minimum_interval_secs) + "},";
  json += "\"power\":{\"valid\":" + String(heltecConfig.hasPower ? "true" : "false") + ",";
  json += "\"saving\":" + String(heltecConfig.power.is_power_saving ? "true" : "false") + ",";
  json += "\"shutdown\":" + String(heltecConfig.power.on_battery_shutdown_after_secs) + ",";
  json += "\"waitBt\":" + String(heltecConfig.power.wait_bluetooth_secs) + ",";
  json += "\"sds\":" + String(heltecConfig.power.sds_secs) + ",";
  json += "\"ls\":" + String(heltecConfig.power.ls_secs) + ",";
  json += "\"wake\":" + String(heltecConfig.power.min_wake_secs) + "},";
  json += "\"network\":{\"valid\":" + String(heltecConfig.hasNetwork ? "true" : "false") + ",";
  json += "\"wifi\":" + String(heltecConfig.network.wifi_enabled ? "true" : "false") + ",";
  json += "\"ssid\":\"" + jsonEscape(heltecConfig.network.wifi_ssid) + "\",";
  json += "\"pskSet\":" + String(strlen(heltecConfig.network.wifi_psk) ? "true" : "false") + ",";
  json += "\"ntp\":\"" + jsonEscape(heltecConfig.network.ntp_server) + "\",";
  json += "\"eth\":" + String(heltecConfig.network.eth_enabled ? "true" : "false") + ",";
  json += "\"ipv6\":" + String(heltecConfig.network.ipv6_enabled ? "true" : "false") + ",";
  json += "\"udp\":" + String((heltecConfig.network.enabled_protocols & meshtastic_Config_NetworkConfig_ProtocolFlags_UDP_BROADCAST) ? "true" : "false") + ",";
  json += "\"syslog\":\"" + jsonEscape(heltecConfig.network.rsyslog_server) + "\"},";
  json += "\"display\":{\"valid\":" + String(heltecConfig.hasDisplay ? "true" : "false") + ",";
  json += "\"screen\":" + String(heltecConfig.display.screen_on_secs) + ",";
  json += "\"carousel\":" + String(heltecConfig.display.auto_screen_carousel_secs) + ",";
  json += "\"north\":" + String(heltecConfig.display.compass_north_top ? "true" : "false") + ",";
  json += "\"flip\":" + String(heltecConfig.display.flip_screen ? "true" : "false") + ",";
  json += "\"units\":" + String((int)heltecConfig.display.units) + ",";
  json += "\"oled\":" + String((int)heltecConfig.display.oled) + ",";
  json += "\"mode\":" + String((int)heltecConfig.display.displaymode) + ",";
  json += "\"bold\":" + String(heltecConfig.display.heading_bold ? "true" : "false") + ",";
  json += "\"wake\":" + String(heltecConfig.display.wake_on_tap_or_motion ? "true" : "false") + ",";
  json += "\"clock12\":" + String(heltecConfig.display.use_12h_clock ? "true" : "false") + ",";
  json += "\"longName\":" + String(heltecConfig.display.use_long_node_name ? "true" : "false") + ",";
  json += "\"bubbles\":" + String(heltecConfig.display.enable_message_bubbles ? "true" : "false") + "},";
  json += "\"bluetooth\":{\"valid\":" + String(heltecConfig.hasBluetooth ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.bluetooth.enabled ? "true" : "false") + ",";
  json += "\"mode\":" + String((int)heltecConfig.bluetooth.mode) + ",";
  json += "\"pin\":" + String(heltecConfig.bluetooth.fixed_pin) + "},";
  json += "\"security\":{\"valid\":" + String(heltecConfig.hasSecurity ? "true" : "false") + ",";
  json += "\"managed\":" + String(heltecConfig.security.is_managed ? "true" : "false") + ",";
  json += "\"serial\":" + String(heltecConfig.security.serial_enabled ? "true" : "false") + ",";
  json += "\"debugLog\":" + String(heltecConfig.security.debug_log_api_enabled ? "true" : "false") + ",";
  json += "\"adminChannel\":" + String(heltecConfig.security.admin_channel_enabled ? "true" : "false") + "},";
  json += "\"modules\":{";
  json += "\"mqtt\":{\"valid\":" + String(heltecConfig.hasMqtt ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.mqtt.enabled ? "true" : "false") + ",";
  json += "\"address\":\"" + jsonEscape(heltecConfig.mqtt.address) + "\",";
  json += "\"username\":\"" + jsonEscape(heltecConfig.mqtt.username) + "\",";
  json += "\"passwordSet\":" + String(strlen(heltecConfig.mqtt.password) ? "true" : "false") + ",";
  json += "\"root\":\"" + jsonEscape(heltecConfig.mqtt.root) + "\",";
  json += "\"encryption\":" + String(heltecConfig.mqtt.encryption_enabled ? "true" : "false") + ",";
  json += "\"json\":" + String(heltecConfig.mqtt.json_enabled ? "true" : "false") + ",";
  json += "\"tls\":" + String(heltecConfig.mqtt.tls_enabled ? "true" : "false") + ",";
  json += "\"proxy\":" + String(heltecConfig.mqtt.proxy_to_client_enabled ? "true" : "false") + ",";
  json += "\"mapReporting\":" + String(heltecConfig.mqtt.map_reporting_enabled ? "true" : "false") + "},";
  json += "\"externalNotification\":{\"valid\":" + String(heltecConfig.hasExternalNotification ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.externalNotification.enabled ? "true" : "false") + ",";
  json += "\"output\":" + String(heltecConfig.externalNotification.output) + ",";
  json += "\"vibra\":" + String(heltecConfig.externalNotification.output_vibra) + ",";
  json += "\"buzzer\":" + String(heltecConfig.externalNotification.output_buzzer) + ",";
  json += "\"outputMs\":" + String(heltecConfig.externalNotification.output_ms) + ",";
  json += "\"nag\":" + String(heltecConfig.externalNotification.nag_timeout) + ",";
  json += "\"active\":" + String(heltecConfig.externalNotification.active ? "true" : "false") + ",";
  json += "\"alertMessage\":" + String(heltecConfig.externalNotification.alert_message ? "true" : "false") + ",";
  json += "\"alertMessageVibra\":" + String(heltecConfig.externalNotification.alert_message_vibra ? "true" : "false") + ",";
  json += "\"alertMessageBuzzer\":" + String(heltecConfig.externalNotification.alert_message_buzzer ? "true" : "false") + ",";
  json += "\"alertBell\":" + String(heltecConfig.externalNotification.alert_bell ? "true" : "false") + ",";
  json += "\"alertBellVibra\":" + String(heltecConfig.externalNotification.alert_bell_vibra ? "true" : "false") + ",";
  json += "\"alertBellBuzzer\":" + String(heltecConfig.externalNotification.alert_bell_buzzer ? "true" : "false") + ",";
  json += "\"usePwm\":" + String(heltecConfig.externalNotification.use_pwm ? "true" : "false") + ",";
  json += "\"useI2s\":" + String(heltecConfig.externalNotification.use_i2s_as_buzzer ? "true" : "false") + "},";
  json += "\"storeForward\":{\"valid\":" + String(heltecConfig.hasStoreForward ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.storeForward.enabled ? "true" : "false") + ",";
  json += "\"heartbeat\":" + String(heltecConfig.storeForward.heartbeat ? "true" : "false") + ",";
  json += "\"records\":" + String(heltecConfig.storeForward.records) + ",";
  json += "\"historyMax\":" + String(heltecConfig.storeForward.history_return_max) + ",";
  json += "\"historyWindow\":" + String(heltecConfig.storeForward.history_return_window) + ",";
  json += "\"server\":" + String(heltecConfig.storeForward.is_server ? "true" : "false") + "},";
  json += "\"rangeTest\":{\"valid\":" + String(heltecConfig.hasRangeTest ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.rangeTest.enabled ? "true" : "false") + ",";
  json += "\"sender\":" + String(heltecConfig.rangeTest.sender) + ",";
  json += "\"save\":" + String(heltecConfig.rangeTest.save ? "true" : "false") + ",";
  json += "\"clear\":" + String(heltecConfig.rangeTest.clear_on_reboot ? "true" : "false") + "},";
  json += "\"telemetry\":{\"valid\":" + String(heltecConfig.hasTelemetry ? "true" : "false") + ",";
  json += "\"deviceEnabled\":" + String(heltecConfig.telemetry.device_telemetry_enabled ? "true" : "false") + ",";
  json += "\"deviceInterval\":" + String(heltecConfig.telemetry.device_update_interval) + ",";
  json += "\"envEnabled\":" + String(heltecConfig.telemetry.environment_measurement_enabled ? "true" : "false") + ",";
  json += "\"envScreen\":" + String(heltecConfig.telemetry.environment_screen_enabled ? "true" : "false") + ",";
  json += "\"envFahrenheit\":" + String(heltecConfig.telemetry.environment_display_fahrenheit ? "true" : "false") + ",";
  json += "\"envInterval\":" + String(heltecConfig.telemetry.environment_update_interval) + ",";
  json += "\"airEnabled\":" + String(heltecConfig.telemetry.air_quality_enabled ? "true" : "false") + ",";
  json += "\"airScreen\":" + String(heltecConfig.telemetry.air_quality_screen_enabled ? "true" : "false") + ",";
  json += "\"airInterval\":" + String(heltecConfig.telemetry.air_quality_interval) + ",";
  json += "\"powerEnabled\":" + String(heltecConfig.telemetry.power_measurement_enabled ? "true" : "false") + ",";
  json += "\"powerScreen\":" + String(heltecConfig.telemetry.power_screen_enabled ? "true" : "false") + ",";
  json += "\"powerInterval\":" + String(heltecConfig.telemetry.power_update_interval) + ",";
  json += "\"healthEnabled\":" + String(heltecConfig.telemetry.health_measurement_enabled ? "true" : "false") + ",";
  json += "\"healthScreen\":" + String(heltecConfig.telemetry.health_screen_enabled ? "true" : "false") + ",";
  json += "\"healthInterval\":" + String(heltecConfig.telemetry.health_update_interval) + "},";
  json += "\"cannedMessage\":{\"valid\":" + String(heltecConfig.hasCannedMessage ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.cannedMessage.enabled ? "true" : "false") + ",";
  json += "\"rotary\":" + String(heltecConfig.cannedMessage.rotary1_enabled ? "true" : "false") + ",";
  json += "\"updown\":" + String(heltecConfig.cannedMessage.updown1_enabled ? "true" : "false") + ",";
  json += "\"pinA\":" + String(heltecConfig.cannedMessage.inputbroker_pin_a) + ",";
  json += "\"pinB\":" + String(heltecConfig.cannedMessage.inputbroker_pin_b) + ",";
  json += "\"pinPress\":" + String(heltecConfig.cannedMessage.inputbroker_pin_press) + ",";
  json += "\"cw\":" + String((int)heltecConfig.cannedMessage.inputbroker_event_cw) + ",";
  json += "\"ccw\":" + String((int)heltecConfig.cannedMessage.inputbroker_event_ccw) + ",";
  json += "\"press\":" + String((int)heltecConfig.cannedMessage.inputbroker_event_press) + ",";
  json += "\"source\":\"" + jsonEscape(heltecConfig.cannedMessage.allow_input_source) + "\",";
  json += "\"sendBell\":" + String(heltecConfig.cannedMessage.send_bell ? "true" : "false") + "},";
  json += "\"audio\":{\"valid\":" + String(heltecConfig.hasAudio ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.audio.codec2_enabled ? "true" : "false") + ",";
  json += "\"bitrate\":" + String((int)heltecConfig.audio.bitrate) + ",";
  json += "\"ptt\":" + String(heltecConfig.audio.ptt_pin) + ",";
  json += "\"ws\":" + String(heltecConfig.audio.i2s_ws) + ",";
  json += "\"sd\":" + String(heltecConfig.audio.i2s_sd) + ",";
  json += "\"din\":" + String(heltecConfig.audio.i2s_din) + ",";
  json += "\"sck\":" + String(heltecConfig.audio.i2s_sck) + "},";
  json += "\"neighborInfo\":{\"valid\":" + String(heltecConfig.hasNeighborInfo ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.neighborInfo.enabled ? "true" : "false") + ",";
  json += "\"interval\":" + String(heltecConfig.neighborInfo.update_interval) + ",";
  json += "\"lora\":" + String(heltecConfig.neighborInfo.transmit_over_lora ? "true" : "false") + "},";
  json += "\"ambientLighting\":{\"valid\":" + String(heltecConfig.hasAmbientLighting ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.ambientLighting.led_state ? "true" : "false") + ",";
  json += "\"current\":" + String(heltecConfig.ambientLighting.current) + ",";
  json += "\"red\":" + String(heltecConfig.ambientLighting.red) + ",";
  json += "\"green\":" + String(heltecConfig.ambientLighting.green) + ",";
  json += "\"blue\":" + String(heltecConfig.ambientLighting.blue) + "},";
  json += "\"detectionSensor\":{\"valid\":" + String(heltecConfig.hasDetectionSensor ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.detectionSensor.enabled ? "true" : "false") + ",";
  json += "\"name\":\"" + jsonEscape(heltecConfig.detectionSensor.name) + "\",";
  json += "\"pin\":" + String(heltecConfig.detectionSensor.monitor_pin) + ",";
  json += "\"minSecs\":" + String(heltecConfig.detectionSensor.minimum_broadcast_secs) + ",";
  json += "\"stateSecs\":" + String(heltecConfig.detectionSensor.state_broadcast_secs) + ",";
  json += "\"trigger\":" + String((int)heltecConfig.detectionSensor.detection_trigger_type) + ",";
  json += "\"sendBell\":" + String(heltecConfig.detectionSensor.send_bell ? "true" : "false") + ",";
  json += "\"pullup\":" + String(heltecConfig.detectionSensor.use_pullup ? "true" : "false") + "},";
  json += "\"paxcounter\":{\"valid\":" + String(heltecConfig.hasPaxcounter ? "true" : "false") + ",";
  json += "\"enabled\":" + String(heltecConfig.paxcounter.enabled ? "true" : "false") + ",";
  json += "\"interval\":" + String(heltecConfig.paxcounter.paxcounter_update_interval) + ",";
  json += "\"wifi\":" + String(heltecConfig.paxcounter.wifi_threshold) + ",";
  json += "\"ble\":" + String(heltecConfig.paxcounter.ble_threshold) + "},";
  json += "\"statusMessage\":{\"valid\":" + String(heltecConfig.hasStatusMessage ? "true" : "false") + ",";
  json += "\"text\":\"" + jsonEscape(heltecConfig.statusMessage.node_status) + "\"}}},";
}

static void appendChannelsStatusJson(String& json) {
  json += "\"channels\":[";
  bool firstChannel = true;
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i].index < 0) continue;
    if (!firstChannel) json += ",";
    firstChannel = false;
    json += "{\"index\":" + String(channels[i].index) + ",";
    json += "\"enabled\":" + String(channels[i].enabled ? "true" : "false") + ",";
    json += "\"role\":\"" + jsonEscape(channels[i].role) + "\",";
    json += "\"name\":\"" + jsonEscape(channels[i].name) + "\",";
    json += "\"uplink\":" + String(channels[i].uplink ? "true" : "false") + ",";
    json += "\"downlink\":" + String(channels[i].downlink ? "true" : "false") + ",";
    json += "\"pskSize\":" + String(channels[i].pskSize) + ",";
    json += "\"hasPositionPrecision\":" + String(channels[i].hasPositionPrecision ? "true" : "false") + ",";
    json += "\"positionPrecision\":" + String(channels[i].positionPrecision) + "}";
  }
  json += "],";
}

static void appendNodesStatusJson(String& json) {
  json += "\"nodes\":[";
  for (size_t i = 0; i < nodeCount; i++) {
    if (i) json += ",";
    json += "{\"num\":\"!" + String(nodes[i].num, HEX) + "\",";
    json += "\"name\":\"" + jsonEscape(nodes[i].name) + "\",";
    json += "\"snr\":" + String(nodes[i].snr, 1) + ",";
    json += "\"age\":" + String((millis() - nodes[i].lastHeardMs) / 1000) + ",";
    json += "\"hasPosition\":" + String(nodes[i].hasPosition ? "true" : "false") + ",";
    json += "\"lat\":" + String(nodes[i].latitude, 6) + ",";
    json += "\"lon\":" + String(nodes[i].longitude, 6) + ",";
    json += "\"alt\":" + String(nodes[i].altitude) + ",";
    json += "\"favorite\":" + String(nodes[i].isFavorite ? "true" : "false") + ",";
    json += "\"positionAge\":" + String(nodes[i].lastPositionMs ? (millis() - nodes[i].lastPositionMs) / 1000 : 0) + "}";
  }
  json += "]";
}

#include "../WebValidation.inc"

static bool requireWebAuth() {
  if (server.authenticate(webUiUser, webUiPass)) {
    if (usingDefaultWebCredentials() && !setupRouteAllowed()) {
      server.send(403, "text/plain", "setup required");
      return false;
    }
    return validateWebNumericArgs();
  }
  server.requestAuthentication(BASIC_AUTH, "Heltec LoRa Interface");
  return false;
}

static String buildStatusJson() {
  sampleLocalBattery();
  refreshSdUsage();
  char rxAge[32];
  if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lus ago", (unsigned long)((millis() - lastByteMs) / 1000));
  else strlcpy(rxAge, "never", sizeof(rxAge));
  String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("off");
  const esp_partition_t* runningPartition = esp_ota_get_running_partition();
  String json;
  json.reserve(STATUS_JSON_RESERVE);
  json = "{";
  appendInterfaceStatusJson(json, wifiIp, runningPartition);
  appendLinkStatusJson(json, rxAge);
  json += "\"myNode\":\"!" + String(stats.myNodeNum, HEX) + "\",";
  json += "\"myNodeName\":\"" + jsonEscape(nodeName(stats.myNodeNum)) + "\",";
  appendBatteryStatusJson(json);
  appendSdStatusJson(json);
  appendTileCacheStatusJson(json);
  json += "\"rx\":" + String(stats.packetsRx) + ",";
  json += "\"tx\":" + String(stats.packetsTx) + ",";
  json += "\"online\":" + String(stats.onlineNodes) + ",";
  json += "\"total\":" + String(stats.totalNodes) + ",";
  appendChatStatusJson(json);
  appendTxStatusJson(json);
  appendMapStatusJson(json);
  appendHeltecConfigStatusJson(json);
  json += "\"i2cProbeSummary\":\"" + jsonEscape(i2cProbeSummary) + "\",";
  json += "\"i2cProbeDetails\":\"" + jsonEscape(i2cProbeDetails) + "\",";
  json += "\"log\":\"" + jsonEscape(eventLog) + "\",";
  appendChannelsStatusJson(json);
  appendNodesStatusJson(json);
  json += "}";
  return json;
}

// Health-only report: omit credentials, channel keys, positions and chat contents.
static String buildFieldReportJson() {
  String json;
  json.reserve(3500);
  json = "{\"reportVersion\":1,";
  json += "\"firmwareVersion\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"gitCommit\":\"" + String(FIRMWARE_GIT_COMMIT) + "\",";
  json += "\"sourceDirty\":" + String(FIRMWARE_SOURCE_DIRTY ? "true" : "false") + ",";
  json += "\"built\":\"" + String(__DATE__) + " " + __TIME__ + "\",";
  json += "\"partition\":\"" + String(esp_ota_get_running_partition()->label) + "\",";
  json += "\"uptimeSec\":" + String(millis() / 1000) + ",";
  json += "\"resetReason\":" + String((int)esp_reset_reason()) + ",";
  json += "\"linkHealth\":\"" + String(linkHealthText()) + "\",";
  json += "\"lastRadioResponseSec\":" + (radioResponseSeen ? String((millis() - lastRadioResponseMs) / 1000) : String("null")) + ",";
  json += "\"framesDecoded\":" + String(framesDecoded) + ",\"decodeErrors\":" + String(decodeErrors) + ",";
  json += "\"rxBytes\":" + String(bytesFromRadio) + ",\"txBytes\":" + String(bytesToRadio) + ",";
  json += "\"frameTimeouts\":" + String(serialFrameTimeouts) + ",\"invalidLengths\":" + String(invalidFrameLengths) + ",";
  json += "\"heartbeats\":" + String(heartbeatCount) + ",\"apiResyncs\":" + String(apiResyncCount) + ",";
  appendLocalGpsHealthJson(json);
  appendBatteryStatusJson(json);
  json += "\"audio\":" + buildToneStatusJson() + ",";
  appendSdStatusJson(json);
  appendTileCacheStatusJson(json);
  json += "\"internalFreeKb\":" + String(heapCapKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + ",";
  json += "\"internalLargestKb\":" + String(heapCapLargestKb(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + ",";
  json += "\"heapMinKb\":" + String(ESP.getMinFreeHeap() / 1024) + ",";
  json += "\"psramFreeKb\":" + String(heapCapKb(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) + ",";
  json += "\"mapWorkerFailures\":" + String(mapRenderWorkerFailures) + ",";
  unsigned queued = 0, sent = 0, acknowledged = 0, failed = 0, unconfirmed = 0;
  for (size_t i = 0; i < txHistoryCount; ++i) {
    switch (FieldStatus::visibleDelivery(txHistory[i].delivery, txHistory[i].sentMs, millis())) {
      case FieldStatus::Delivery::Queued: ++queued; break;
      case FieldStatus::Delivery::Sent: ++sent; break;
      case FieldStatus::Delivery::Acknowledged: ++acknowledged; break;
      case FieldStatus::Delivery::Failed: ++failed; break;
      default: ++unconfirmed; break;
    }
  }
  json += "\"delivery\":{\"tracked\":" + String(txHistoryCount) + ",\"queued\":" + String(queued) + ",";
  json += "\"sent\":" + String(sent) + ",\"acknowledged\":" + String(acknowledged) + ",";
  json += "\"failed\":" + String(failed) + ",\"unconfirmed\":" + String(unconfirmed) + "}}";
  return json;
}

static void handleFieldReport() {
  if (!requireWebAuth()) return;
  sampleLocalBattery();
  refreshSdUsage();
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Disposition", "attachment; filename=field-diagnostics.json");
  server.send(200, "application/json", buildFieldReportJson());
}

static void handleRetryTx() {
  if (!requireWebAuth()) return;
  int64_t id = 0;
  if (!server.hasArg("id") || !Reliability::parseInteger(server.arg("id").c_str(), 1, UINT32_MAX, id)) {
    server.send(400, "text/plain", "invalid packet ID");
    return;
  }
  bool ok = retryTx((uint32_t)id);
  server.send(ok ? 200 : 409, "text/plain", ok ? "Retry queued. See TX history for delivery." : "Retry unavailable: already retried, still pending, acknowledged, or expired from history.");
}

static void handleStatus() {
  if (!requireWebAuth()) return;
  server.send(200, "application/json", buildStatusJson());
}

static void handleStatusSnapshot() {
  if (!requireWebAuth()) return;
  if (sdDownloadPathActive(SD_STATUS_SNAPSHOT_PATH)) {
    server.send(409, "text/plain", "wait for the status snapshot download to finish");
    return;
  }
  String json = buildStatusJson();
  if (!sdStorage.available) {
    server.send(503, "text/plain", "SD card not available");
    return;
  }
  bool ok = false;
  {
    SdLock lock;
    if (!lock) {
      server.send(503, "text/plain", "SD card busy");
      return;
    }
    if (SD_MMC.exists(SD_STATUS_SNAPSHOT_PATH)) SD_MMC.remove(SD_STATUS_SNAPSHOT_PATH);
    File file = SD_MMC.open(SD_STATUS_SNAPSHOT_PATH, FILE_WRITE);
    if (!file) {
      sdStorage.writeErrors++;
      strlcpy(sdStorage.status, "snapshot open failed", sizeof(sdStorage.status));
      server.send(500, "text/plain", "snapshot open failed");
      return;
    }
    ok = file.print(json);
    file.close();
    if (ok) {
      sdStorage.writes++;
      strlcpy(sdStorage.status, "snapshot saved", sizeof(sdStorage.status));
    } else {
      sdStorage.writeErrors++;
      strlcpy(sdStorage.status, "snapshot write failed", sizeof(sdStorage.status));
    }
  }
  if (ok) {
    appendLine(eventLog, LOG_SIZE, "[sd] status snapshot saved\n");
    server.send(200, "application/json", json);
  } else {
    server.send(500, "text/plain", "snapshot write failed");
  }
}

struct SdDownload {
  NetworkClient client;
  File file;
  const char* path = nullptr;
  size_t remaining = 0;
  size_t buffered = 0;
  size_t offset = 0;
  uint32_t lastProgressMs = 0;
  bool active = false;
  uint8_t* memory = nullptr; // PSRAM snapshot for web tiles; no SD lock during network sends.
  size_t memoryOffset = 0;
  uint8_t buffer[1024];
};
static SdDownload sdDownload;

static bool sdDownloadPathActive(const char* path) {
  return sdDownload.active && sdDownload.path && strcmp(sdDownload.path, path) == 0;
}

static void finishSdDownload() {
  // If another SD worker is busy, retry closing on the next main-loop pass.
  if (!sdDownload.memory) {
    SdLock lock(0);
    if (!lock) return;
    sdDownload.file.close();
  }
  if (sdDownload.memory) heap_caps_free(sdDownload.memory);
  sdDownload.memory = nullptr;
  sdDownload.memoryOffset = 0;
  sdDownload.client.stop();
  sdDownload.active = false;
  sdDownload.path = nullptr;
}

static void serviceSdDownload() {
  if (!sdDownload.active) return;
  if (!wifiEnabled || !sdDownload.client.connected() ||
      millis() - sdDownload.lastProgressMs >= 10000 ||
      (sdDownload.remaining == 0 && sdDownload.offset == sdDownload.buffered)) {
    finishSdDownload();
    return;
  }
  if (sdDownload.memory) {
    size_t requested = min(sdDownload.remaining, (size_t)4096);
    int written = lwip_send(sdDownload.client.fd(), sdDownload.memory + sdDownload.memoryOffset, requested, MSG_DONTWAIT);
    if (written > 0) {
      sdDownload.memoryOffset += written;
      sdDownload.remaining -= written;
      sdDownload.lastProgressMs = millis();
    } else if (written == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) finishSdDownload();
    return;
  }
  if (sdDownload.offset == sdDownload.buffered) {
    SdLock lock(0);
    if (!lock) return;
    size_t requested = min(sdDownload.remaining, sizeof(sdDownload.buffer));
    sdDownload.buffered = sdDownload.file.read(sdDownload.buffer, requested);
    sdDownload.offset = 0;
    sdDownload.remaining -= sdDownload.buffered;
    if (sdDownload.buffered == 0) {
      finishSdDownload();
      return;
    }
  }
  int written = lwip_send(sdDownload.client.fd(), sdDownload.buffer + sdDownload.offset,
                          sdDownload.buffered - sdDownload.offset, MSG_DONTWAIT);
  if (written > 0) {
    sdDownload.offset += written;
    sdDownload.lastProgressMs = millis();
  } else if (written == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
    finishSdDownload();
  }
}

static void handleSdDownload(const char* path, const char* downloadName, const char* contentType) {
  if (!requireWebAuth()) return;
  if (sdDownload.active) {
    server.send(503, "text/plain", "another SD download is in progress");
    return;
  }
  if (!sdStorage.available) {
    server.send(503, "text/plain", "SD card not available");
    return;
  }
  {
    SdLock lock(0);
    if (!lock) {
      server.send(503, "text/plain", "SD card busy");
      return;
    }
    if (!SD_MMC.exists(path)) {
      server.send(404, "text/plain", "file not found");
      return;
    }
    sdDownload.file = SD_MMC.open(path, FILE_READ);
    if (!sdDownload.file) {
      server.send(500, "text/plain", "open failed");
      return;
    }
    sdDownload.remaining = sdDownload.file.size();
  }
  sdDownload.buffered = sdDownload.offset = 0;
  sdDownload.lastProgressMs = millis();
  sdDownload.client = server.client();
  sdDownload.path = path;
  sdDownload.active = true;
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + downloadName + "\"");
  server.setContentLength(sdDownload.remaining);
  server.send(200, contentType, "");
}

#include "WebMap.inc"
#include "WebMapTests.inc"

static void handleSend() {
  if (!requireWebAuth()) return;
  if (!server.hasArg("msg") || server.arg("msg").length() == 0 || server.arg("msg").length() > 233) {
    server.send(400, "text/plain", "message must contain 1-233 UTF-8 bytes");
    return;
  }
  String channel = server.hasArg("channel") ? server.arg("channel") : String("public");
  if (server.hasArg("to") || channel == "direct") {
    uint32_t to = parseNodeAddress(server.hasArg("to") ? server.arg("to").c_str() : "");
    if (!isDirectAddress(to)) {
      server.send(400, "text/plain", "missing direct target");
      return;
    }
    bool ok = sendDirectTextMessage(server.arg("msg").c_str(), to);
    server.send(ok ? 200 : 500, "text/plain", ok ? "direct queued" : "direct send failed");
    return;
  }
  int8_t channelIndex = PUBLIC_CHANNEL_INDEX;
  int64_t parsed = 0;
  if (channel == "public" || channel == "primary") {
    channelIndex = PUBLIC_CHANNEL_INDEX;
  } else if (channel == "secondary" || channel == "family" || channel == "private") {
    channelIndex = privateChannelIndex;
  } else {
    String index = channel.startsWith("ch:") ? channel.substring(3) : channel;
    if (!Reliability::parseInteger(index.c_str(), 0, MAX_CHANNELS - 1, parsed)) {
      server.send(400, "text/plain", "invalid channel");
      return;
    }
    channelIndex = (int8_t)parsed;
  }
  ChannelRecord* selected = channelIndex >= 0 ? channelRecordByIndex(channelIndex) : nullptr;
  if (!selected || !selected->enabled) {
    server.send(400, "text/plain", "channel is disabled or not loaded; request radio config first");
    return;
  }
  bool ok = sendTextMessage(server.arg("msg").c_str(), channelIndex);
  server.send(ok ? 200 : 500, "text/plain", ok ? "queued" : "send failed");
}

static void handleSerialCmd() {
  if (!requireWebAuth()) return;
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "missing cmd");
    return;
  }
  String cmd = server.arg("cmd") + "\n";
  SerialLoRa.write((const uint8_t*)cmd.c_str(), cmd.length());
  server.send(200, "text/plain", "sent");
}

static void handleToneStatus() {
  if (!requireWebAuth()) return;
  server.send(200, "application/json", buildToneStatusJson());
}

static void handleI2cProbe() {
  if (!requireWebAuth()) return;
  String json = buildI2cProbeJson();
  appendLine(eventLog, LOG_SIZE, "[i2c] probe completed\n");
  server.send(200, "application/json", json);
}

static void handleToneTest() {
  if (!requireWebAuth()) return;

  if (server.hasArg("volume")) {
    toneVolumePercent = formU8("volume");
    prefs.putUChar("toneVol", toneVolumePercent);
  }
  // Accept legacy boost/profile arguments without restoring retired gain modes
  // or writing unchanged compatibility values to NVS on each preview request.
  if (server.hasArg("message")) {
    int8_t style = toneStyleIndexFromText(server.arg("message"));
    if (style >= 0) {
      toneMessageStyle = clampToneStyle((uint8_t)style);
      prefs.putUChar("toneMsg", (uint8_t)toneMessageStyle);
    }
  }
  if (server.hasArg("direct")) {
    int8_t style = toneStyleIndexFromText(server.arg("direct"));
    if (style >= 0) {
      toneDirectStyle = clampToneStyle((uint8_t)style);
      prefs.putUChar("toneDir", (uint8_t)toneDirectStyle);
    }
  }

  applyToneVolume();
  updateToneDropdowns();
  updateToneVolumeLabel();

  ToneStyle styleToPlay = toneMessageStyle;
  if (server.hasArg("style")) {
    int8_t requestedStyle = toneStyleIndexFromText(server.arg("style"));
    if (requestedStyle >= 0) styleToPlay = clampToneStyle((uint8_t)requestedStyle);
  }
  bool accepted = queueToneStyle(styleToPlay);
  server.send(accepted ? 202 : 409, "application/json", buildToneStatusJson());
}

static void handleToneCodecWrite() {
  if (!requireWebAuth()) return;
  server.send(410, "text/plain", "Direct codec writes are disabled. Use the sound volume and tone controls.");
}

static bool webBoolArg(const String& value) {
  String v = value;
  v.trim();
  v.toLowerCase();
  return v == "1" || v == "true" || v == "yes" || v == "on" || v == "favorite";
}

static void handleNodeFavorite() {
  if (!requireWebAuth()) return;
  String nodeArg = server.hasArg("node") ? server.arg("node") : server.arg("num");
  uint32_t nodeNum = parseNodeAddress(nodeArg.c_str());
  if (!isDirectAddress(nodeNum)) {
    server.send(400, "text/plain", "missing or invalid node");
    return;
  }

  bool favorite = server.hasArg("favorite") ? webBoolArg(server.arg("favorite")) : true;
  bool ok = sendHeltecFavoriteNode(nodeNum, favorite);
  if (!ok) {
    server.send(500, "text/plain", "favorite command failed");
    return;
  }

  if (NodeRecord* node = findNode(nodeNum)) {
    node->isFavorite = favorite;
  }
  markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD);
  refreshNodeDetailFavoriteButton();
  refreshNodeList(true);
  refreshMessageDiagView();

  String json = "{";
  json += "\"node\":\"!" + String(nodeNum, HEX) + "\",";
  json += "\"favorite\":" + String(favorite ? "true" : "false") + ",";
  json += "\"sent\":true}";
  server.send(200, "application/json", json);
}

static void handleRoot() {
  if (!requireWebAuth()) return;
  server.send_P(200, "text/html", WEB_UI_HTML);
}

static void handleFirmwareUpload() {
  if (usbTileUploadActive() || usbSdExportActive()) {
    otaUploadOk = false;
    return;
  }
  if (!server.authenticate(webUiUser, webUiPass)) {
    otaUploadOk = false;
    return;
  }
  if (usingDefaultWebCredentials()) {
    otaUploadOk = false;
    return;
  }

  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    otaUploadOk = false;
    Serial.printf("[ota] upload start: %s\n", upload.filename.c_str());
    appendLine(eventLog, LOG_SIZE, "[ota] firmware upload started\n");

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Serial.printf("[ota] begin failed: %u\n", Update.getError());
      appendLine(eventLog, LOG_SIZE, "[ota] begin failed\n");
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError()) {
      size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        Serial.printf("[ota] short write: %u/%u error=%u\n",
                      (unsigned)written,
                      (unsigned)upload.currentSize,
                      Update.getError());
      }
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError() && Update.end(true)) {
      otaUploadOk = true;
      Serial.printf("[ota] upload complete: %u bytes\n", (unsigned)upload.totalSize);
      appendLine(eventLog, LOG_SIZE, "[ota] firmware upload complete, reboot pending\n");
    } else {
      otaUploadOk = false;
      Serial.printf("[ota] end failed: %u\n", Update.getError());
      appendLine(eventLog, LOG_SIZE, "[ota] firmware upload failed\n");
      Update.abort();
    }

  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    otaUploadOk = false;
    Update.abort();
    Serial.println("[ota] upload aborted");
    appendLine(eventLog, LOG_SIZE, "[ota] firmware upload aborted\n");
  }
}

static void handleFirmwareUpdateDone() {
  if (!requireWebAuth()) return;

  if (otaUploadOk) {
    otaRestartPending = true;
    otaRestartAtMs = millis() + 1200;
    server.send(200, "text/plain", "Firmware update complete. Rebooting...");
  } else {
    server.send(500, "text/plain", String("Firmware update failed. Error ") + Update.getError());
  }
}


static void registerWebRoutes() {
  registerWebMapRoutes();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/diagnostics/report", HTTP_GET, handleFieldReport);
  server.on("/tx/retry", HTTP_POST, handleRetryTx);
  server.on("/tone/status", HTTP_GET, handleToneStatus);
  server.on("/i2c/probe", HTTP_POST, handleI2cProbe);
  server.on("/tone/test", HTTP_GET, handleToneTest);
  server.on("/tone/test", HTTP_POST, handleToneTest);
  server.on("/tone/codec", HTTP_POST, handleToneCodecWrite);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/serial_cmd", HTTP_POST, handleSerialCmd);
  server.on("/firmware", HTTP_POST, handleFirmwareUpdateDone, handleFirmwareUpload);
  server.on("/gps/share", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    const String action = server.arg("action");
    if (action != "setup" && action != "on" && action != "off") {
      server.send(400, "text/plain", "action must be setup, on or off"); return;
    }
    if (gpsRelaySetupBusy()) { server.send(409, "text/plain", "GPS sharing setup is busy"); return; }
    const bool ok = action == "setup" ? prepareGpsRelay() : setGpsRelayEnabled(action == "on");
    server.send(ok ? (action == "setup" ? 202 : 200) : 409, "text/plain",
                ok ? "GPS sharing request accepted; read /status for current state" : gpsRelaySetupError());
  });
  server.on("/interface", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    String newPass = server.arg("webPass");
    if (usingDefaultWebCredentials() && newPass.length() < 8) {
      server.send(400, "text/plain", "new password required");
      return;
    }
    if (newPass.length() && newPass.length() < 8) {
      server.send(400, "text/plain", "password must be at least 8 characters");
      return;
    }
    bool wasEnabled = wifiEnabled;
    if (wasEnabled) stopWifi();
    saveInterfaceSettings(server.arg("deviceName"),
                          server.arg("hostname"),
                          server.arg("apSsid"),
                          formU8("apChannel", interfaceApChannel),
                          server.arg("webUser"),
                          newPass);
    if (server.arg("deviceName").length()) {
      sendHeltecOwnerName(server.arg("deviceName"), server.arg("deviceName").substring(0, min<size_t>(4, server.arg("deviceName").length())));
    }
    if (wasEnabled) startWifi();
    appendLine(eventLog, LOG_SIZE, "[web] interface settings saved\n");
    server.send(200, "text/plain", "interface settings saved");
  });
  server.on("/sd/events", HTTP_GET, []() { handleSdDownload(SD_EVENTS_PATH, "events.log", "text/plain"); });
  server.on("/sd/public", HTTP_GET, []() { handleSdDownload(SD_PUBLIC_CHAT_PATH, "public_chat.log", "text/plain"); });
  server.on("/sd/secondary", HTTP_GET, []() { handleSdDownload(SD_SECONDARY_CHAT_PATH, "secondary_chat.log", "text/plain"); });
  server.on("/sd/private", HTTP_GET, []() { handleSdDownload(SD_SECONDARY_CHAT_PATH, "secondary_chat.log", "text/plain"); });
  server.on("/sd/direct", HTTP_GET, []() { handleSdDownload(SD_DIRECT_CHAT_PATH, "direct_messages.log", "text/plain"); });
  server.on("/sd/positions", HTTP_GET, []() { handleSdDownload(SD_POSITIONS_PATH, "positions.csv", "text/csv"); });
  server.on("/sd/mapcache", HTTP_GET, []() { handleSdDownload(SD_MAP_CACHE_PATH, "map_cache.bin", "application/octet-stream"); });
  server.on("/sd/last-location", HTTP_GET, []() { handleSdDownload(SD_LAST_LOCATION_PATH, "last_location.txt", "text/plain"); });
  server.on("/sd/status-snapshot", HTTP_GET, []() { handleSdDownload(SD_STATUS_SNAPSHOT_PATH, "status_snapshot.json", "application/json"); });
  server.on("/sd/snapshot", HTTP_POST, handleStatusSnapshot);
  server.on("/sd/mount", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    if (mapRenderWorkerRunning || mapBenchmarkRunning || usbTileUploadActive() || usbSdExportActive()) {
      server.send(409, "text/plain", "wait for map rendering, benchmark or USB SD transfer to finish");
      return;
    }
    if (sdDownload.active) {
      server.send(409, "text/plain", "wait for the SD download to finish");
      return;
    }
    initSdStorage();
    server.send(sdStorage.available ? 200 : 503, "text/plain", sdStorage.status);
  });
  server.on("/nodes/clear", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    clearNodeList();
    server.send(200, "text/plain", "node list reset");
  });
  server.on("/nodes/favorite", HTTP_POST, handleNodeFavorite);
  server.on("/config", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendConfigRequest();
    server.send(ok ? 200 : 500, "text/plain", ok ? "requested" : "request failed");
  });
  server.on("/heltec/reboot", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecReboot(1);
    server.send(ok ? 200 : 500, "text/plain", ok ? "reboot requested" : "reboot request failed");
  });
  server.on("/heltec/lora", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecLoraConfig(server.arg("region"), server.arg("preset"), formU8("hop"), (int8_t)formI32("tx"));
    server.send(ok ? 200 : 500, "text/plain", ok ? "lora config sent" : "lora config failed");
  });
  server.on("/heltec/serial", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    uint32_t rxd = server.hasArg("rxd") ? formU32("rxd") : 38;
    uint32_t txd = server.hasArg("txd") ? formU32("txd") : 39;
    String baud = server.arg("baud");
    String mode = server.arg("mode");
    bool ok = sendHeltecSerialConfig(server.arg("enabled") == "1",
                                     rxd,
                                     txd,
                                     baud.length() ? baud : String("115200"),
                                     mode.length() ? mode : String("PROTO"),
                                     server.hasArg("timeout") ? formU32("timeout") : 0,
                                     server.arg("echo") == "1",
                                     server.arg("override") == "1");
    server.send(ok ? 200 : 500, "text/plain", ok ? "serial config sent" : "serial config failed");
  });
  server.on("/heltec/name", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecOwnerName(server.arg("name"), server.arg("short"));
    server.send(ok ? 200 : 400, "text/plain", ok ? "name sent" : "missing name");
  });
  server.on("/heltec/device", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecDeviceConfig(server.arg("role"),
                                     server.arg("rebroadcast"),
                                     formU32("nodeInfo"),
                                     server.arg("tz"),
                                     server.arg("ledOff") == "1",
                                     server.arg("buzzer"));
    server.send(ok ? 200 : 500, "text/plain", ok ? "device config sent" : "device config failed");
  });
  server.on("/heltec/position", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecPositionConfig(server.arg("gpsEnabled") == "1",
                                       server.arg("gpsMode"),
                                       server.arg("fixed") == "1",
                                       server.arg("smart") == "1",
                                       formU32("broadcast"),
                                       formU32("gpsUpdate"),
                                       formU32("gpsAttempt"),
                                       formU32("smartMeters"),
                                       formU32("smartSecs"));
    server.send(ok ? 200 : 500, "text/plain", ok ? "position config sent" : "position config failed");
  });
  server.on("/heltec/power", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecPowerConfig(server.arg("saving") == "1",
                                    formU32("shutdown"),
                                    formU32("waitBt"),
                                    formU32("sds"),
                                    formU32("ls"),
                                    formU32("wake"));
    server.send(ok ? 200 : 500, "text/plain", ok ? "power config sent" : "power config failed");
  });
  server.on("/heltec/network", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecNetworkConfig();
    server.send(ok ? 200 : 500, "text/plain", ok ? "network config sent" : "network config failed");
  });
  server.on("/heltec/display", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecDisplayConfig();
    server.send(ok ? 200 : 500, "text/plain", ok ? "display config sent" : "display config failed");
  });
  server.on("/heltec/bluetooth", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecBluetoothConfig();
    server.send(ok ? 200 : 500, "text/plain", ok ? "bluetooth config sent" : "bluetooth config failed");
  });
  server.on("/heltec/module", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecModuleConfig(server.arg("section"));
    server.send(ok ? 200 : 500, "text/plain", ok ? "module config sent" : "module config failed");
  });
  server.on("/heltec/timezone", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecTimezone(server.arg("tz"));
    server.send(ok ? 200 : 400, "text/plain", ok ? "timezone sent" : "missing timezone");
  });
  server.on("/heltec/save", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecCommit();
    server.send(ok ? 200 : 500, "text/plain", ok ? "save sent" : "save failed");
  });
  server.on("/heltec/channel", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecChannelConfig(formU8("index"),
                                      server.arg("role"),
                                      server.arg("name"),
                                      server.arg("psk"),
                                      server.arg("uplink") == "1",
                                      server.arg("downlink") == "1",
                                      server.hasArg("positionPrecision") ? formI32("positionPrecision") : -1);
    server.send(ok ? 200 : 500, "text/plain", ok ? "channel sent" : "channel failed");
  });
}

// Internal implementation module; included once by the main sketch.
static int localGpsSatellitesInView() {
  TinyGPSCustom* value = nullptr;
  if (gpsGpInView.isValid() && gpsGpInView.age() <= 30000) value = &gpsGpInView;
  if (gpsGnInView.isValid() && gpsGnInView.age() <= 30000 && (!value || gpsGnInView.age() < value->age())) value = &gpsGnInView;
  return value ? atoi(value->value()) : -1;
}

static void appendLocalGpsHealthJson(String& json) {
  bool usedKnown = localGps.satellites.isValid() && localGps.satellites.age() <= 30000;
  int inView = localGpsSatellitesInView();
  json += "\"gpsRxPin\":" + String(GPS_RX_PIN) + ",\"gpsBaud\":" + String(GPS_BAUD) + ",";
  json += "\"localGpsBytes\":" + String(gpsBytesFromLocal) + ",";
  json += "\"localGpsByteAgeMs\":" + (gpsBytesFromLocal ? String((uint32_t)(millis() - lastLocalGpsByteMs)) : String("null")) + ",";
  json += "\"localGpsValidSentences\":" + String(localGps.passedChecksum()) + ",";
  json += "\"localGpsFailedChecksum\":" + String(localGps.failedChecksum()) + ",";
  json += "\"localGpsSentences\":" + String(localGps.sentencesWithFix()) + ",";
  json += "\"localGpsSatsUsed\":" + (usedKnown ? String(localGps.satellites.value()) : String("null")) + ",";
  json += "\"localGpsSatsInView\":" + (inView >= 0 ? String(inView) : String("null")) + ",";
  json += "\"localGpsFreshFix\":" + String(localGps.location.isValid() && localGps.location.age() <= 30000 ? "true" : "false") + ",";
}

static void printLocalGpsDiagnostics() {
  String report = "{"; appendLocalGpsHealthJson(report);
  report += "\"heltecPositionConfigKnown\":" + String(heltecConfig.hasPosition ? "true" : "false") + ",";
  report += "\"heltecGpsMode\":" + String((int)heltecConfig.position.gps_mode) + ",";
  report += "\"heltecGpsRxPin\":" + String(heltecConfig.position.rx_gpio) + ",\"heltecGpsTxPin\":" + String(heltecConfig.position.tx_gpio) + ",";
  report += "\"meshPositionMessages\":" + String(positionPackets) + "}";
  Serial.println(String("[GPS] ") + report);
}

static void updateLocalGpsStats() {
  if (!localGps.location.isValid()) return;

  localGpsHasFix = true;
  gpsStats.valid = true;
  gpsStats.from = stats.myNodeNum;
  gpsStats.latitude = localGps.location.lat();
  gpsStats.longitude = localGps.location.lng();
  gpsStats.altitude = localGps.altitude.isValid() ? (int32_t)localGps.altitude.meters() : 0;
  gpsStats.hasAltitudeHae = false;
  gpsStats.hasGeoidalSeparation = false;
  if (localGps.satellites.isValid()) {
    gpsStats.sats = localGps.satellites.value();
    gpsStats.hasSats = true;
  }
  gpsStats.fixQuality = 1;
  gpsStats.hasFixQuality = true;
  gpsStats.fixType = gpsStats.hasSats && gpsStats.sats >= 4 ? 3 : 2;
  gpsStats.hasFixType = true;
  if (localGps.hdop.isValid()) {
    gpsStats.hdop = localGps.hdop.value();
    gpsStats.hasHdop = true;
  }
  if (localGps.speed.isValid()) {
    gpsStats.groundSpeed = (uint32_t)(localGps.speed.mps() * 100.0);
    gpsStats.hasGroundSpeed = true;
  }
  if (localGps.course.isValid()) {
    gpsStats.groundTrack = (uint32_t)(localGps.course.deg() * 100.0);
    gpsStats.hasGroundTrack = true;
  }
  strlcpy(gpsStats.sourceKind, "CYD GPS UART", sizeof(gpsStats.sourceKind));
  gpsStats.lastUpdateMs = millis();
  markUiDirty(UI_DIRTY_MAP | UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD | UI_DIRTY_SYSTEM);
  mapRenderPending = true;
  saveLastLocationToSd(gpsStats.latitude, gpsStats.longitude, gpsStats.altitude);

  if (stats.myNodeNum != 0) {
    NodeRecord* node = findOrCreateNode(stats.myNodeNum);
    if (node) {
      node->hasPosition = true;
      node->latitude = gpsStats.latitude;
      node->longitude = gpsStats.longitude;
      node->altitude = gpsStats.altitude;
      node->lastPositionMs = millis();
    }
  }

  if (millis() - lastLocalGpsFixLogMs > 30000) {
    lastLocalGpsFixLogMs = millis();
    char line[160];
    snprintf(line, sizeof(line), "[cyd gps] fix %.6f, %.6f\n", gpsStats.latitude, gpsStats.longitude);
    appendLine(eventLog, LOG_SIZE, line);
    logPositionToSd(stats.myNodeNum, "CYD GPS UART", gpsStats.latitude, gpsStats.longitude, gpsStats.altitude);
  }
}

static void pollLocalGps() {
  while (SerialGPS.available()) {
    char c = (char)SerialGPS.read();
    gpsBytesFromLocal++;
    lastLocalGpsByteMs = millis();
    if (localGps.encode(c) && localGps.location.isUpdated()) {
      updateLocalGpsStats();
    }
  }
}

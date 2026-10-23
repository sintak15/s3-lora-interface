// Internal implementation module; included once by the main sketch.
static GpsUbx::Decoder gpsUbx;
static GpsConfig::Controller gpsConfig;
static uint32_t gpsUbxByteMs = 0, gpsUbxReplies = 0, gpsReportedOperation = 0;

static const char* gpsControlState() {
  using namespace GpsConfig;
  if (gpsConfig.phase == Phase::WaitAck) return "awaiting-ack";
  if (gpsConfig.busy()) return gpsConfig.operation == Operation::Apply ? "verifying-settings" : "reading-settings";
  if (gpsConfig.result == Result::Applied) return "settings-applied";
  if (gpsConfig.result == Result::Saved) return "save-acknowledged";
  if (gpsConfig.result == Result::Read) return "settings-read";
  if (gpsConfig.result == Result::PartialRead) return "partial-read";
  if (gpsConfig.error == Error::Timeout) return "timeout";
  if (gpsConfig.error == Error::Rejected) return "rejected";
  if (gpsConfig.error == Error::Mismatch) return "readback-mismatch";
  if (gpsConfig.error == Error::WriteFailed) return "write-failed";
  return "idle";
}

static void serviceGpsConfig() {
  gpsConfig.tick(millis());
  if (gpsConfig.readyToSend()) {
    const auto packet = gpsConfig.outgoing();
    if (GPS_TX_PIN < 0 || !packet.size) gpsConfig.sent(false, millis());
    else if (SerialGPS.availableForWrite() >= (int)packet.size)
      gpsConfig.sent(SerialGPS.write(packet.bytes, packet.size) == packet.size, millis());
  }
  if (!gpsConfig.busy() && gpsConfig.serial != gpsReportedOperation) {
    gpsReportedOperation = gpsConfig.serial;
    Serial.printf("[GPS] config %s%s%s%s%s\n", gpsControlState(),
                  gpsConfig.error == GpsConfig::Error::None ? "" : ": ",
                  gpsConfig.error == GpsConfig::Error::None ? "" : GpsConfig::fieldName(gpsConfig.errorField),
                  gpsConfig.error == GpsConfig::Error::None ? "" : " - ", GpsConfig::errorText(gpsConfig.error));
    forcePageRefresh = true;
  }
}

static bool handleGpsControlCommand(const char* command) {
  bool readAll = strcmp(command, "gps config") == 0;
  bool save = strcmp(command, "gps save") == 0;
  bool rate = strcmp(command, "gps rate") == 0 || strncmp(command, "gps rate ", 9) == 0;
  if (!readAll && !save && !rate) return false;
  if (gpsConfig.busy()) { Serial.println("[GPS] command pending; wait for completion"); return true; }
  bool accepted = false;
  if (readAll) accepted = gpsConfig.read(millis());
  else if (save) accepted = gpsConfig.save(millis());
  else if (strcmp(command, "gps rate") == 0) accepted = gpsConfig.read(millis(), true);
  else {
    unsigned hz = strcmp(command, "gps rate 1") == 0 ? 1 : strcmp(command, "gps rate 2") == 0 ? 2 :
                  strcmp(command, "gps rate 5") == 0 ? 5 : 0;
    if (!hz) { Serial.println("[GPS] usage: gps rate [1|2|5], gps config, gps save"); return true; }
    GpsConfig::Changes change;
    change.periodMs = 1000 / hz;
    accepted = gpsConfig.apply(change, millis());
  }
  Serial.println(accepted ? "[GPS] receiver command queued" :
                 String("[GPS] ") + (save ? "Read/apply settings before saving." : GpsConfig::errorText(gpsConfig.error)));
  forcePageRefresh = true;
  return true;
}

static void serviceGpsUbxReply() {
  ++gpsUbxReplies;
  gpsConfig.receive(gpsUbx.cls, gpsUbx.id, gpsUbx.payload, gpsUbx.length, millis());
  forcePageRefresh = true;
}

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
  json += "\"gpsTxPin\":" + String(GPS_TX_PIN) + ",\"gpsControlState\":\"" + gpsControlState() + "\",";
  json += "\"gpsMeasurementPeriodMs\":" + (gpsConfig.current.has(GpsConfig::Field::Rate) ? String(GpsConfig::read16(gpsConfig.current.rate)) : String("null")) + ",";
  json += "\"gpsUbxReplies\":" + String(gpsUbxReplies) + ",\"gpsUbxChecksumErrors\":" + String(gpsUbx.bad) + ",";
  const auto& receiver = gpsConfig.current;
  json += "\"gpsConfigBusy\":" + String(gpsConfig.busy() ? "true" : "false") + ",";
  json += "\"gpsConfigKnownMask\":" + String(receiver.known) + ",";
  json += "\"gpsNavigationModel\":" + (receiver.has(GpsConfig::Field::Navigation) ? String(receiver.navigation[2]) : String("null")) + ",";
  json += "\"gpsSbasEnabled\":" + String(receiver.has(GpsConfig::Field::Sbas) ? ((receiver.sbas[0] & 1) ? "true" : "false") : "null") + ",";
  json += "\"gpsReceiverPowerMode\":" + (receiver.has(GpsConfig::Field::Power) ? String(receiver.power[1]) : String("null")) + ",";
  json += "\"gpsNmeaProfile\":\"" + String(GpsConfig::profileName(receiver.profile())) + "\",";
  json += "\"gpsConfigCanSave\":" + String(!gpsConfig.busy() && gpsConfig.canSave ? "true" : "false") + ",";
  json += "\"gpsConfigError\":\"" + String(GpsConfig::errorText(gpsConfig.error)) + "\",";
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
  const uint32_t now=millis();
  if(gpsUbx.state && (uint32_t)(now-gpsUbxByteMs)>250) gpsUbx.reset();
  while (SerialGPS.available()) {
    char c = (char)SerialGPS.read();
    gpsBytesFromLocal++;
    lastLocalGpsByteMs = millis();
    bool binary=gpsUbx.consume((uint8_t)c);
    gpsUbxByteMs=millis();
    if(gpsUbx.ready) serviceGpsUbxReply();
    if(binary) continue;
    if (localGps.encode(c) && localGps.location.isUpdated()) {
      updateLocalGpsStats();
    }
  }
  serviceGpsConfig();
}

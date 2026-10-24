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
  if (strcmp(command, "gps share on") == 0 || strcmp(command, "gps share off") == 0) {
    bool ok = setGpsRelayEnabled(strcmp(command, "gps share on") == 0);
    Serial.println(ok ? "[GPS] sharing preference saved" : "[GPS] sharing preference could not be saved");
    return true;
  }
  if (strcmp(command, "gps share setup") == 0) {
    bool ok = prepareGpsRelay();
    Serial.println(ok ? "[GPS] reading Heltec settings before setup" : String("[GPS] ") + gpsRelaySetupError());
    return true;
  }
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

static TinyGPSCustom* recentGpsField(TinyGPSCustom& gp, TinyGPSCustom& gn, uint32_t maxAge) {
  TinyGPSCustom* field = gp.isValid() && gp.age() <= maxAge ? &gp : nullptr;
  if (gn.isValid() && gn.age() <= maxAge && (!field || gn.age() < field->age())) field = &gn;
  return field;
}

static bool readGpsFixStatus() {
  // TinyGPS commits these custom fields only after a checksum-valid sentence.
  // Invalid sentences do not clear TinyGPS's previous location.isValid().
  TinyGPSCustom* fields[] = {&gpsGpStatus, &gpsGnStatus, &gpsGpQuality, &gpsGnQuality, &gpsGpFixType, &gpsGnFixType};
  bool usableSentence = true;
  for (uint8_t i = 0; i < 6; ++i) {
    if (!fields[i]->isUpdated()) continue;
    const char* value = fields[i]->value();
    if (!GpsRelay::usableStatus(i / 2, value)) { gpsRelayFix.valid = false; usableSentence = false; }
  }
  return usableSentence;
}

static void captureGpsRelayFix() {
  GpsRelay::Fix fix;
  const double latitude = localGps.location.lat(), longitude = localGps.location.lng();
  TinyGPSCustom* quality = recentGpsField(gpsGpQuality, gpsGnQuality, GpsRelay::FIX_MAX_AGE_MS);
  TinyGPSCustom* type = recentGpsField(gpsGpFixType, gpsGnFixType, GpsRelay::FIX_MAX_AGE_MS);
  // A later RMC 'A' must not revive a fix that a recent GGA/GSA rejected.
  if ((quality && !GpsRelay::usableStatus(1, quality->value())) ||
      (type && !GpsRelay::usableStatus(2, type->value()))) { gpsRelayFix.valid = false; return; }
  if (!isfinite(latitude) || !isfinite(longitude) || latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
    gpsRelayFix.valid = false; return;
  }
  fix.valid = true; fix.capturedMs = millis(); fix.sequence = gpsRelayFix.sequence + 1;
  if (!fix.sequence) fix.sequence = 1;
  fix.latitude = (int32_t)lround(latitude * 10000000.0);
  fix.longitude = (int32_t)lround(longitude * 10000000.0);
  if (localGps.date.isValid() && localGps.time.isValid() &&
      localGps.date.age() <= GpsRelay::FIX_MAX_AGE_MS && localGps.time.age() <= GpsRelay::FIX_MAX_AGE_MS) {
    fix.epoch = GpsRelay::utcEpoch(localGps.date.year(), localGps.date.month(), localGps.date.day(),
                                 localGps.time.hour(), localGps.time.minute(), localGps.time.second());
  }
  if (localGps.altitude.isValid() && localGps.altitude.age() <= GpsRelay::FIX_MAX_AGE_MS) {
    fix.hasAltitude = true; fix.altitude = (int32_t)lround(localGps.altitude.meters());
  }
  if (localGps.speed.isValid() && localGps.speed.age() <= GpsRelay::FIX_MAX_AGE_MS) {
    fix.hasSpeed = true; fix.speed = (uint32_t)lround(localGps.speed.kmph());
  }
  if (localGps.course.isValid() && localGps.course.age() <= GpsRelay::FIX_MAX_AGE_MS && localGps.course.value() < 36000) {
    fix.hasTrack = true; fix.track = localGps.course.value() * 1000U;
  }
  if (localGps.hdop.isValid() && localGps.hdop.age() <= GpsRelay::FIX_MAX_AGE_MS) fix.hdop = localGps.hdop.value();
  if (quality) fix.quality = (uint32_t)atoi(quality->value());
  if (type) { const int value = atoi(type->value()); if (value == 2 || value == 3) fix.type = value; }
  const int inView = localGpsSatellitesInView();
  if (inView >= 0) fix.satellitesInView = inView;
  gpsRelayFix = fix;
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
  json += "\"localGpsFreshFix\":" + String(gpsRelayFix.fresh(millis()) ? "true" : "false") + ",";
  json += "\"gpsShareEnabled\":" + String(gpsRelay.enabled ? "true" : "false") + ",";
  json += "\"gpsShareState\":\"" + String(GpsRelay::stateText(gpsRelayState())) + "\",";
  json += "\"gpsShareSetupBusy\":" + String(gpsRelaySetupBusy() ? "true" : "false") + ",";
  json += "\"gpsShareSetupError\":\"" + String(gpsRelaySetupError()) + "\",";
  json += "\"gpsShareQueued\":" + String(gpsRelay.queued) + ",\"gpsShareAccepted\":" + String(gpsRelay.accepted) + ",";
  json += "\"gpsShareRejected\":" + String(gpsRelay.rejected) + ",\"gpsShareTimeouts\":" + String(gpsRelay.timeouts) + ",";
  json += "\"gpsShareWriteFailures\":" + String(gpsRelay.writeFailures) + ",";
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
  captureGpsRelayFix();
  if (!gpsRelayFix.valid) return;

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
  gpsStats.fixQuality = gpsRelayFix.quality;
  gpsStats.hasFixQuality = gpsRelayFix.quality != 0;
  gpsStats.fixType = gpsRelayFix.type;
  gpsStats.hasFixType = gpsRelayFix.type != 0;
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
    if (localGps.encode(c)) {
      const bool usableSentence = readGpsFixStatus();
      if (localGps.location.isUpdated()) {
        if (usableSentence) updateLocalGpsStats();
        else localGps.location.lat(); // consume a rejected update, never revive it on a later GSV
      }
    }
  }
  serviceGpsConfig();
}

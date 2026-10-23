// Internal implementation module; included once by the main sketch.
static GpsUbx::Decoder gpsUbx;
static uint32_t gpsUbxByteMs=0, gpsCommandMs=0, gpsUbxReplies=0;
static bool gpsCommandPending=false, gpsRateKnown=false;
static uint16_t gpsRateMs=0;
static const char* gpsControlState="idle";

static bool sendGpsRatePacket(unsigned hz) {
  if (GPS_TX_PIN < 0) return false;
  const auto packet=GpsUbx::ratePacket(hz);
  return packet.size && SerialGPS.write(packet.bytes,packet.size)==packet.size;
}

static bool handleGpsControlCommand(const char* command) {
  if (strcmp(command,"gps rate")!=0 && strncmp(command,"gps rate ",9)!=0) return false;
  if (gpsCommandPending) { Serial.println("[GPS] rate command pending; wait for ACK or timeout"); return true; }
  unsigned hz=0;
  if (strcmp(command,"gps rate")==0) hz=0;
  else if(strcmp(command,"gps rate 1")==0) hz=1;
  else if(strcmp(command,"gps rate 2")==0) hz=2;
  else if(strcmp(command,"gps rate 5")==0) hz=5;
  else { Serial.println("[GPS] usage: gps rate [1|2|5]"); return true; }
  if(!sendGpsRatePacket(hz)) { Serial.println("[GPS] UART TX unavailable or write failed"); return true; }
  gpsRateKnown=false;
  gpsCommandMs=millis(); gpsCommandPending=true;
  gpsControlState=hz ? "awaiting-ack" : "awaiting-rate";
  Serial.printf("[GPS] CFG-RATE %s sent; awaiting receiver response\n",hz ? "set" : "poll");
  if(hz>1) Serial.println("[GPS] shared stream remains 9600 baud; higher rates can exceed NMEA bandwidth");
  return true;
}

static void serviceGpsUbxReply() {
  ++gpsUbxReplies;
  if(gpsUbx.cls==6 && gpsUbx.id==8 && gpsUbx.length==6) {
    gpsRateMs=(uint16_t)gpsUbx.payload[0] | ((uint16_t)gpsUbx.payload[1]<<8);
    gpsRateKnown=true;
    if(gpsCommandPending && strcmp(gpsControlState,"awaiting-rate")==0) {
      gpsCommandPending=false; gpsControlState="rate-received";
    }
    Serial.printf("[GPS] receiver measurement period %u ms\n",(unsigned)gpsRateMs);
  } else if(gpsCommandPending && strcmp(gpsControlState,"awaiting-ack")==0 &&
            gpsUbx.cls==5 && gpsUbx.length==2 && gpsUbx.payload[0]==6 && gpsUbx.payload[1]==8 &&
            (gpsUbx.id==0 || gpsUbx.id==1)) {
    if(gpsUbx.id==0) { gpsCommandPending=false; gpsControlState="rejected"; }
    else if(sendGpsRatePacket(0)) {
      gpsControlState="awaiting-rate"; gpsCommandMs=millis();
    } else { gpsCommandPending=false; gpsControlState="acknowledged-poll-failed"; }
    Serial.printf("[GPS] CFG-RATE %s\n",gpsUbx.id ? "acknowledged; querying receiver rate" : "rejected");
  }
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
  json += "\"gpsTxPin\":" + String(GPS_TX_PIN) + ",\"gpsControlState\":\"" + gpsControlState + "\",";
  json += "\"gpsMeasurementPeriodMs\":" + (gpsRateKnown ? String(gpsRateMs) : String("null")) + ",";
  json += "\"gpsUbxReplies\":" + String(gpsUbxReplies) + ",\"gpsUbxChecksumErrors\":" + String(gpsUbx.bad) + ",";
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
  if(gpsCommandPending && (uint32_t)(now-gpsCommandMs)>2000) {
    gpsCommandPending=false; gpsControlState="timeout";
    Serial.println("[GPS] receiver response timeout; check CYD GPIO2 -> GPS RX");
  }
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
}

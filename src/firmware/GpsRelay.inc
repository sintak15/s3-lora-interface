// Internal GPS sharing over the existing PROTO UART; included after Radio.inc.
enum class GpsRelaySetup : uint8_t { IDLE, READ, APPLY_WAIT, VERIFY };
static GpsRelaySetup gpsRelaySetup = GpsRelaySetup::IDLE;
static GpsRelay::SetupTimer gpsRelaySetupTimer;
static uint32_t gpsRelaySetupId = 0, gpsRelaySetupStartedMs = 0;
static bool gpsRelaySawPosition = false, gpsRelaySawSerial = false, gpsRelayHadLink = false;
static char gpsRelayError[96] = "";

static bool gpsRelayLinkReady() {
  return stats.myNodeNum && stats.myNodeNum != BROADCAST_ADDR && radioResponseSeen &&
         (uint32_t)(millis() - lastRadioResponseMs) < GpsRelay::LINK_MAX_AGE_MS;
}

static bool gpsRelaySerialReady() {
  const auto& serial = heltecConfig.serial;
  return heltecConfig.hasSerial && serial.enabled &&
         serial.mode == meshtastic_ModuleConfig_SerialConfig_Serial_Mode_PROTO &&
         (serial.baud == meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_115200 ||
          serial.baud == meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_DEFAULT) &&
         serial.rxd == 38 && serial.txd == 39;
}

static bool gpsRelaySetupBusy() { return gpsRelaySetup != GpsRelaySetup::IDLE; }
static const char* gpsRelaySetupError() { return gpsRelayError; }

static bool setGpsRelayEnabled(bool enabled) {
  if (gpsRelaySetupBusy()) return false;
  if (gpsRelay.enabled != enabled && prefs.putBool("gpsShare", enabled) == 0) return false;
  gpsRelay.setEnabled(enabled);
  gpsRelayError[0] = '\0';
  forcePageRefresh = true;
  return true;
}

static void failGpsRelaySetup(const char* message) {
  gpsRelaySetup = GpsRelaySetup::IDLE;
  strlcpy(gpsRelayError, message, sizeof(gpsRelayError));
  forcePageRefresh = true;
}

static bool readGpsRelayConfig(GpsRelaySetup phase) {
  gpsRelaySetup = phase;
  gpsRelaySetupId = nextConfigRequestId;
  gpsRelaySetupStartedMs = millis();
  gpsRelaySawPosition = gpsRelaySawSerial = false;
  if (!sendConfigRequest()) { failGpsRelaySetup("Heltec settings request could not be sent."); return false; }
  return true;
}

static bool prepareGpsRelay() {
  if (gpsRelaySetupBusy()) return false;
  if (!gpsRelayLinkReady()) { failGpsRelaySetup("Connect the Heltec and wait for its node information."); return false; }
  if (!setGpsRelayEnabled(false)) { failGpsRelaySetup("Sharing preference could not be saved."); return false; }
  gpsRelaySetupTimer = {};
  // Always obtain a new complete snapshot before changing any Heltec settings.
  return readGpsRelayConfig(GpsRelaySetup::READ);
}

static void onGpsRelayConfig(bool position) {
  if (!gpsRelaySetupBusy()) return;
  if (position) gpsRelaySawPosition = true;
  else gpsRelaySawSerial = true;
}

static void onGpsRelayConfigComplete(uint32_t id) {
  if ((gpsRelaySetup != GpsRelaySetup::READ && gpsRelaySetup != GpsRelaySetup::VERIFY) || id != gpsRelaySetupId) return;
  if (!gpsRelaySawPosition || !gpsRelaySawSerial) {
    failGpsRelaySetup("Heltec did not return position and serial settings."); return;
  }
  if (!gpsRelaySerialReady()) { failGpsRelaySetup("Heltec serial must use PROTO, 115200, RX38 / TX39."); return; }
  if (GpsRelay::physicalGpsAbsent(heltecConfig.position)) {
    gpsRelaySetup = GpsRelaySetup::IDLE;
    if (!setGpsRelayEnabled(true)) failGpsRelaySetup("Heltec ready; sharing preference could not be saved.");
    else appendLine(eventLog, LOG_SIZE, "[gps] Heltec external GPS settings verified; sharing enabled\n");
    return;
  }
  if (gpsRelaySetup == GpsRelaySetup::VERIFY) {
    // The admin packet is handled asynchronously. A completed config stream
    // can still contain the old snapshot; retry only the read, never the write.
    if (gpsRelaySetupTimer.expired(millis())) failGpsRelaySetup("Heltec settings did not match; sharing remains off.");
    else { gpsRelaySetup = GpsRelaySetup::APPLY_WAIT; gpsRelaySetupTimer.wait(millis()); }
    return;
  }
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_position_tag;
  admin.set_config.payload_variant.position = GpsRelay::externalConfig(heltecConfig.position);
  if (!sendLocalAdmin(admin)) { failGpsRelaySetup("Heltec position settings could not be sent."); return; }
  gpsRelaySetupTimer.beginWrite(millis());
  gpsRelaySetup = GpsRelaySetup::APPLY_WAIT;
  gpsRelaySetupId = 0;
  // Let the router process the queued admin packet before requesting readback.
}

static void resetGpsRelayLink() {
  gpsRelay.resetLink();
  heltecConfig.hasPosition = heltecConfig.hasSerial = false;
  if (gpsRelaySetup == GpsRelaySetup::VERIFY || gpsRelaySetup == GpsRelaySetup::APPLY_WAIT) {
    // Position settings normally reboot the Heltec. Resume readback after it
    // reconnects without resending the configuration write.
    gpsRelaySetup = GpsRelaySetup::APPLY_WAIT;
    gpsRelaySetupId = 0; gpsRelaySetupTimer.wait(millis());
    gpsRelaySawPosition = gpsRelaySawSerial = false;
  } else if (gpsRelaySetupBusy()) failGpsRelaySetup("Heltec restarted while reading settings; retry after reconnection.");
}

static GpsRelay::State gpsRelayState() {
  const bool known = heltecConfig.hasPosition && heltecConfig.hasSerial && !gpsRelaySetupBusy();
  return gpsRelay.state(gpsRelayFix, millis(), gpsRelayLinkReady(), known,
                        gpsRelaySerialReady() && GpsRelay::physicalGpsAbsent(heltecConfig.position));
}

static bool encodeGpsRelayPacket(uint8_t* bytes, size_t capacity, size_t& size, uint32_t& id) {
  if (!gpsRelayLinkReady() || !gpsRelayFix.fresh(millis())) return false;
  id = allocateMeshPacketId();
  meshtastic_ToRadio message = GpsRelay::localMessage(stats.myNodeNum, id);
  if (!message.which_payload_variant) return false;
  auto& packet = message.packet;
  // No want_ack / want_response: the radio's queue result acknowledges ingestion.
  const auto position = GpsRelay::position(gpsRelayFix);
  pb_ostream_t payload = pb_ostream_from_buffer(packet.decoded.payload.bytes, sizeof(packet.decoded.payload.bytes));
  if (!pb_encode(&payload, meshtastic_Position_fields, &position)) return false;
  packet.decoded.payload.size = payload.bytes_written;
  pb_ostream_t stream = pb_ostream_from_buffer(bytes, capacity);
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &message)) return false;
  size = stream.bytes_written;
  return true;
}

static void serviceGpsRelay() {
  const uint32_t now = millis();
  gpsRelay.tick(now);
  if (gpsRelaySetupBusy() && gpsRelaySetupTimer.expired(now))
    failGpsRelaySetup("Heltec settings did not match before timeout; sharing remains off.");
  if ((gpsRelaySetup == GpsRelaySetup::READ || gpsRelaySetup == GpsRelaySetup::VERIFY) &&
      (uint32_t)(now - gpsRelaySetupStartedMs) >= 15000)
    failGpsRelaySetup("Heltec settings read timed out; reconnect and retry setup.");
  const bool link = gpsRelayLinkReady();
  if (!link && gpsRelayHadLink && (gpsRelay.enabled || gpsRelaySetupBusy())) resetGpsRelayLink();
  gpsRelayHadLink = link;
  if (link && gpsRelaySetup == GpsRelaySetup::APPLY_WAIT && gpsRelaySetupTimer.ready(now))
    readGpsRelayConfig(GpsRelaySetup::VERIFY);
  if (!gpsRelay.enabled || gpsRelaySetupBusy()) return;
  const bool known = heltecConfig.hasPosition && heltecConfig.hasSerial;
  if (link && !known && (uint32_t)(now - lastConfigRequestMs) >= 30000) sendConfigRequest();
  if (!gpsRelay.due(gpsRelayFix, now, link, known,
                    gpsRelaySerialReady() && GpsRelay::physicalGpsAbsent(heltecConfig.position))) return;
  uint8_t bytes[256]; size_t size = 0; uint32_t id = 0;
  if (!encodeGpsRelayPacket(bytes, sizeof(bytes), size, id)) {
    gpsRelay.sent(0, gpsRelayFix.sequence, now, false); return;
  }
  if (SerialLoRa.availableForWrite() < (int)(size + 4)) return;
  gpsRelay.sent(id, gpsRelayFix.sequence, now, writeStreamFrame(bytes, size));
  forcePageRefresh = true;
}

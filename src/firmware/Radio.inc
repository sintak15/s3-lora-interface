// Internal implementation module; included once by the main sketch.
static NodeRecord* findOrCreateNode(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num) return &nodes[i];
  }
  if (nodeCount >= MAX_NODES) return nullptr;
  nodeListClearedMs = 0;
  nodes[nodeCount].num = num;
  snprintf(nodes[nodeCount].name, sizeof(nodes[nodeCount].name), "!%08lX", (unsigned long)num);
  nodes[nodeCount].lastHeardMs = millis();
  markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD | UI_DIRTY_MAP);
  return &nodes[nodeCount++];
}

static const char* nodeName(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num && nodes[i].name[0]) return nodes[i].name;
  }
  static char fallback[16];
  snprintf(fallback, sizeof(fallback), "!%08lX", (unsigned long)num);
  return fallback;
}

static const char* channelName(uint8_t index) {
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i].enabled && channels[i].index == (int8_t)index && channels[i].name[0]) {
      if (index == PUBLIC_CHANNEL_INDEX && isPublicChannelName(channels[i].name)) return "Public";
      return channels[i].name;
    }
  }
  static char fallback[16];
  if (index == PUBLIC_CHANNEL_INDEX) return "Public";
  snprintf(fallback, sizeof(fallback), "channel %u", (unsigned)index);
  return fallback;
}

static const char* publicChannelLabel() {
  return channelName(PUBLIC_CHANNEL_INDEX);
}

static const char* privateChannelLabel() {
  uint8_t index = privateChannelIndex >= 0 ? (uint8_t)privateChannelIndex : 1;
  return channelName(index);
}

static void formatChannelPrompt(const char* channel, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  snprintf(out, outSize, "%s message", channel && channel[0] ? channel : "Channel");
}

static void formatNoChannelChat(const char* channel, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  snprintf(out, outSize, "No %s chat yet", channel && channel[0] ? channel : "channel");
}

static void refreshChannelUiLabels() {
  char text[64];
  const char* publicName = publicChannelLabel();
  const char* privateName = privateChannelLabel();
  if (lblTilePublic) lv_label_set_text(lblTilePublic, publicName);
  if (lblTilePrivate) lv_label_set_text(lblTilePrivate, privateName);
  if (lblPublicChatTitle) {
    snprintf(text, sizeof(text), "%s Chat", publicName);
    lv_label_set_text(lblPublicChatTitle, text);
  }
  if (lblPrivateChatTitle) {
    snprintf(text, sizeof(text), "%s Chat", privateName);
    lv_label_set_text(lblPrivateChatTitle, text);
  }
  if (lblPublicChannelStats) {
    snprintf(text, sizeof(text), "Ch %u: %s", (unsigned)PUBLIC_CHANNEL_INDEX, publicName);
    setReadoutText(lblPublicChannelStats, text);
  }
  if (lblPrivateChannelStats) {
    uint8_t privateIndex = privateChannelIndex >= 0 ? (uint8_t)privateChannelIndex : 1;
    snprintf(text, sizeof(text), "Ch %u: %s", (unsigned)privateIndex, privateName);
    setReadoutText(lblPrivateChannelStats, text);
  }
  if (lblPrivateChannelHint) {
    uint8_t privateIndex = privateChannelIndex >= 0 ? (uint8_t)privateChannelIndex : 1;
    snprintf(text, sizeof(text), "Ch %u on radio: %s", (unsigned)privateIndex, privateName);
    setReadoutText(lblPrivateChannelHint, text);
  }
  if (taPublicInput) {
    formatChannelPrompt(publicName, text, sizeof(text));
    lv_textarea_set_placeholder_text(taPublicInput, text);
  }
  if (taFamilyInput) {
    formatChannelPrompt(privateName, text, sizeof(text));
    lv_textarea_set_placeholder_text(taFamilyInput, text);
  }
  markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_CONFIG | UI_DIRTY_DASHBOARD);
  refreshDashboardLabels();
}

static bool isPublicChannelName(const char* name) {
  if (!name || !name[0]) return false;
  return strcasecmp(name, "public") == 0 ||
         strcasecmp(name, "primary") == 0 ||
         strcasecmp(name, "mediumfast") == 0 ||
         strcasecmp(name, "medium fast") == 0 ||
         strcasecmp(name, "longfast") == 0 ||
         strcasecmp(name, "long fast") == 0 ||
         strcasecmp(name, "default") == 0;
}

static ChannelRecord* channelRecordByIndex(uint8_t index) {
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i].enabled && channels[i].index == (int8_t)index) return &channels[i];
  }
  return nullptr;
}

static bool hasEnabledChannelRecords() {
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i].enabled) return true;
  }
  return false;
}

static void refreshPrivateChannelIndex() {
  privateChannelIndex = -1;
  int8_t firstSecondary = -1;
  int8_t channelOneSecondary = -1;
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (!channels[i].enabled || strcmp(channels[i].role, "SECONDARY") != 0) continue;
    if (firstSecondary < 0) firstSecondary = channels[i].index;
    if (channels[i].index == 1) channelOneSecondary = channels[i].index;
  }
  privateChannelIndex = channelOneSecondary >= 0 ? channelOneSecondary : firstSecondary;
}

static void updateChannelRecord(const meshtastic_Channel& channel) {
  if (channel.index < 0 || channel.index >= (int8_t)MAX_CHANNELS) return;
  ChannelRecord& record = channels[channel.index];
  record.index = channel.index;
  record.enabled = channel.role != meshtastic_Channel_Role_DISABLED;
  switch (channel.role) {
    case meshtastic_Channel_Role_PRIMARY:
      strlcpy(record.role, "PRIMARY", sizeof(record.role));
      break;
    case meshtastic_Channel_Role_SECONDARY:
      strlcpy(record.role, "SECONDARY", sizeof(record.role));
      break;
    default:
      strlcpy(record.role, "DISABLED", sizeof(record.role));
      break;
  }
  if (channel.has_settings && channel.settings.name[0]) {
    strlcpy(record.name, channel.settings.name, sizeof(record.name));
  } else if (channel.index == PUBLIC_CHANNEL_INDEX) {
    strlcpy(record.name, "primary", sizeof(record.name));
  } else {
    record.name[0] = '\0';
  }
  record.uplink = channel.has_settings && channel.settings.uplink_enabled;
  record.downlink = channel.has_settings && channel.settings.downlink_enabled;
  record.pskSize = channel.has_settings ? channel.settings.psk.size : 0;
  memset(record.psk, 0, sizeof(record.psk));
  if (channel.has_settings && record.pskSize > 0) {
    size_t copyLen = min<size_t>(record.pskSize, sizeof(record.psk));
    memcpy(record.psk, channel.settings.psk.bytes, copyLen);
    record.pskSize = copyLen;
  }
  record.hasPositionPrecision = channel.has_settings && channel.settings.has_module_settings;
  record.positionPrecision = record.hasPositionPrecision ? channel.settings.module_settings.position_precision : 0;
  refreshPrivateChannelIndex();

  char line[96];
  snprintf(line, sizeof(line), "[radio] channel %d: %s\n", record.index, record.name[0] ? record.name : "(unnamed)");
  appendLine(eventLog, LOG_SIZE, line);
  refreshChannelUiLabels();
}

static const char* loraRegionName(meshtastic_Config_LoRaConfig_RegionCode value) {
  switch (value) {
    case meshtastic_Config_LoRaConfig_RegionCode_US: return "US";
    case meshtastic_Config_LoRaConfig_RegionCode_EU_433: return "EU_433";
    case meshtastic_Config_LoRaConfig_RegionCode_EU_868: return "EU_868";
    case meshtastic_Config_LoRaConfig_RegionCode_CN: return "CN";
    case meshtastic_Config_LoRaConfig_RegionCode_JP: return "JP";
    case meshtastic_Config_LoRaConfig_RegionCode_ANZ: return "ANZ";
    case meshtastic_Config_LoRaConfig_RegionCode_KR: return "KR";
    case meshtastic_Config_LoRaConfig_RegionCode_TW: return "TW";
    case meshtastic_Config_LoRaConfig_RegionCode_RU: return "RU";
    case meshtastic_Config_LoRaConfig_RegionCode_IN: return "IN";
    default: return "UNSET";
  }
}

static const char* loraPresetName(meshtastic_Config_LoRaConfig_ModemPreset value) {
  switch (value) {
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_SLOW: return "LONG_SLOW";
    case meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_FAST: return "MEDIUM_FAST";
    case meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_SLOW: return "MEDIUM_SLOW";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_FAST: return "SHORT_FAST";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_SLOW: return "SHORT_SLOW";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_TURBO: return "SHORT_TURBO";
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_MODERATE: return "LONG_MODERATE";
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_TURBO: return "LONG_TURBO";
    default: return "LONG_FAST";
  }
}

static const char* deviceRoleName(meshtastic_Config_DeviceConfig_Role value) {
  switch (value) {
    case meshtastic_Config_DeviceConfig_Role_CLIENT_MUTE: return "CLIENT_MUTE";
    case meshtastic_Config_DeviceConfig_Role_ROUTER: return "ROUTER";
    case meshtastic_Config_DeviceConfig_Role_ROUTER_CLIENT: return "ROUTER_CLIENT";
    case meshtastic_Config_DeviceConfig_Role_REPEATER: return "REPEATER";
    case meshtastic_Config_DeviceConfig_Role_TRACKER: return "TRACKER";
    case meshtastic_Config_DeviceConfig_Role_SENSOR: return "SENSOR";
    case meshtastic_Config_DeviceConfig_Role_TAK: return "TAK";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_HIDDEN: return "CLIENT_HIDDEN";
    case meshtastic_Config_DeviceConfig_Role_LOST_AND_FOUND: return "LOST_AND_FOUND";
    case meshtastic_Config_DeviceConfig_Role_TAK_TRACKER: return "TAK_TRACKER";
    case meshtastic_Config_DeviceConfig_Role_ROUTER_LATE: return "ROUTER_LATE";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_BASE: return "CLIENT_BASE";
    default: return "CLIENT";
  }
}

static const char* rebroadcastName(meshtastic_Config_DeviceConfig_RebroadcastMode value) {
  switch (value) {
    case meshtastic_Config_DeviceConfig_RebroadcastMode_ALL_SKIP_DECODING: return "ALL_SKIP_DECODING";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_LOCAL_ONLY: return "LOCAL_ONLY";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_KNOWN_ONLY: return "KNOWN_ONLY";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_NONE: return "NONE";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_CORE_PORTNUMS_ONLY: return "CORE_PORTNUMS_ONLY";
    default: return "ALL";
  }
}

static const char* buzzerName(meshtastic_Config_DeviceConfig_BuzzerMode value) {
  switch (value) {
    case meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED: return "DISABLED";
    case meshtastic_Config_DeviceConfig_BuzzerMode_NOTIFICATIONS_ONLY: return "NOTIFICATIONS_ONLY";
    case meshtastic_Config_DeviceConfig_BuzzerMode_SYSTEM_ONLY: return "SYSTEM_ONLY";
    case meshtastic_Config_DeviceConfig_BuzzerMode_DIRECT_MSG_ONLY: return "DIRECT_MSG_ONLY";
    default: return "ALL_ENABLED";
  }
}

static const char* gpsModeName(meshtastic_Config_PositionConfig_GpsMode value) {
  switch (value) {
    case meshtastic_Config_PositionConfig_GpsMode_DISABLED: return "DISABLED";
    case meshtastic_Config_PositionConfig_GpsMode_NOT_PRESENT: return "NOT_PRESENT";
    default: return "ENABLED";
  }
}

static const char* serialBaudName(meshtastic_ModuleConfig_SerialConfig_Serial_Baud value) {
  switch (value) {
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_9600: return "9600";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_19200: return "19200";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_38400: return "38400";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_57600: return "57600";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_230400: return "230400";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_460800: return "460800";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_576000: return "576000";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_921600: return "921600";
    default: return "115200";
  }
}

static const char* serialModeName(meshtastic_ModuleConfig_SerialConfig_Serial_Mode value) {
  switch (value) {
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_SIMPLE: return "SIMPLE";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_TEXTMSG: return "TEXTMSG";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_NMEA: return "NMEA";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_CALTOPO: return "CALTOPO";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_WS85: return "WS85";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_VE_DIRECT: return "VE_DIRECT";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_MS_CONFIG: return "MS_CONFIG";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_LOG: return "LOG";
    case meshtastic_ModuleConfig_SerialConfig_Serial_Mode_LOGTEXT: return "LOGTEXT";
    default: return "PROTO";
  }
}

static void cacheHeltecConfig(const meshtastic_Config& config) {
  heltecConfig.lastConfigMs = millis();
  switch (config.which_payload_variant) {
    case meshtastic_Config_lora_tag:
      heltecConfig.lora = config.payload_variant.lora;
      heltecConfig.hasLora = true;
      appendPacketEvent("[config] LoRa config cached\n");
      break;
    case meshtastic_Config_device_tag:
      heltecConfig.device = config.payload_variant.device;
      heltecConfig.hasDevice = true;
      appendPacketEvent("[config] device config cached\n");
      break;
    case meshtastic_Config_position_tag:
      heltecConfig.position = config.payload_variant.position;
      heltecConfig.hasPosition = true;
      onGpsRelayConfig(true);
      appendPacketEvent("[config] position config cached\n");
      break;
    case meshtastic_Config_power_tag:
      heltecConfig.power = config.payload_variant.power;
      heltecConfig.hasPower = true;
      appendPacketEvent("[config] power config cached\n");
      break;
    case meshtastic_Config_network_tag:
      heltecConfig.network = config.payload_variant.network;
      heltecConfig.hasNetwork = true;
      appendPacketEvent("[config] network config cached\n");
      break;
    case meshtastic_Config_display_tag:
      heltecConfig.display = config.payload_variant.display;
      heltecConfig.hasDisplay = true;
      appendPacketEvent("[config] display config cached\n");
      break;
    case meshtastic_Config_bluetooth_tag:
      heltecConfig.bluetooth = config.payload_variant.bluetooth;
      heltecConfig.hasBluetooth = true;
      appendPacketEvent("[config] bluetooth config cached\n");
      break;
    case meshtastic_Config_security_tag:
      heltecConfig.security = config.payload_variant.security;
      heltecConfig.hasSecurity = true;
      appendPacketEvent("[config] security config cached\n");
      break;
    default:
      appendPacketEvent("[config] unsupported config cached counter only\n");
      break;
  }
}

static void cacheHeltecModuleConfig(const meshtastic_ModuleConfig& moduleConfig) {
  heltecConfig.lastModuleMs = millis();
  switch (moduleConfig.which_payload_variant) {
    case meshtastic_ModuleConfig_mqtt_tag:
      heltecConfig.mqtt = moduleConfig.payload_variant.mqtt;
      heltecConfig.hasMqtt = true;
      appendPacketEvent("[config] MQTT module cached\n");
      break;
    case meshtastic_ModuleConfig_serial_tag:
      heltecConfig.serial = moduleConfig.payload_variant.serial;
      heltecConfig.hasSerial = true;
      onGpsRelayConfig(false);
      appendPacketEvent("[config] serial module cached\n");
      break;
    case meshtastic_ModuleConfig_external_notification_tag:
      heltecConfig.externalNotification = moduleConfig.payload_variant.external_notification;
      heltecConfig.hasExternalNotification = true;
      appendPacketEvent("[config] external notification module cached\n");
      break;
    case meshtastic_ModuleConfig_store_forward_tag:
      heltecConfig.storeForward = moduleConfig.payload_variant.store_forward;
      heltecConfig.hasStoreForward = true;
      appendPacketEvent("[config] store and forward module cached\n");
      break;
    case meshtastic_ModuleConfig_range_test_tag:
      heltecConfig.rangeTest = moduleConfig.payload_variant.range_test;
      heltecConfig.hasRangeTest = true;
      appendPacketEvent("[config] range test module cached\n");
      break;
    case meshtastic_ModuleConfig_telemetry_tag:
      heltecConfig.telemetry = moduleConfig.payload_variant.telemetry;
      heltecConfig.hasTelemetry = true;
      appendPacketEvent("[config] telemetry module cached\n");
      break;
    case meshtastic_ModuleConfig_canned_message_tag:
      heltecConfig.cannedMessage = moduleConfig.payload_variant.canned_message;
      heltecConfig.hasCannedMessage = true;
      appendPacketEvent("[config] canned message module cached\n");
      break;
    case meshtastic_ModuleConfig_audio_tag:
      heltecConfig.audio = moduleConfig.payload_variant.audio;
      heltecConfig.hasAudio = true;
      appendPacketEvent("[config] audio module cached\n");
      break;
    case meshtastic_ModuleConfig_neighbor_info_tag:
      heltecConfig.neighborInfo = moduleConfig.payload_variant.neighbor_info;
      heltecConfig.hasNeighborInfo = true;
      appendPacketEvent("[config] neighbor info module cached\n");
      break;
    case meshtastic_ModuleConfig_ambient_lighting_tag:
      heltecConfig.ambientLighting = moduleConfig.payload_variant.ambient_lighting;
      heltecConfig.hasAmbientLighting = true;
      appendPacketEvent("[config] ambient lighting module cached\n");
      break;
    case meshtastic_ModuleConfig_detection_sensor_tag:
      heltecConfig.detectionSensor = moduleConfig.payload_variant.detection_sensor;
      heltecConfig.hasDetectionSensor = true;
      appendPacketEvent("[config] detection sensor module cached\n");
      break;
    case meshtastic_ModuleConfig_paxcounter_tag:
      heltecConfig.paxcounter = moduleConfig.payload_variant.paxcounter;
      heltecConfig.hasPaxcounter = true;
      appendPacketEvent("[config] paxcounter module cached\n");
      break;
    case meshtastic_ModuleConfig_statusmessage_tag:
      heltecConfig.statusMessage = moduleConfig.payload_variant.statusmessage;
      heltecConfig.hasStatusMessage = true;
      appendPacketEvent("[config] status message module cached\n");
      break;
    default:
      appendPacketEvent("[config] unsupported module cached counter only\n");
      break;
  }
}

static bool isPrivateChannel(uint8_t index) {
  ChannelRecord* record = channelRecordByIndex(index);
  if (record) {
    if (strcmp(record->role, "PRIMARY") == 0 || isPublicChannelName(record->name)) return false;
    if (strcmp(record->role, "SECONDARY") == 0) return true;
  }
  if (privateChannelIndex >= 0) return index == privateChannelIndex;
  return !hasEnabledChannelRecords() && index == 1;
}

static void refreshChatViews(bool force) {
  char emptyText[48];
  bool updated = false;
  bool visible = currentPage == pagePublicChat || currentPage == pagePrivateChat || currentPage == pageDirectChat;
  if (!visible) return;
  if (!force && !isUiDirty(UI_DIRTY_CHAT)) return;
  if (currentPage == pagePublicChat && taPublicChat) {
    formatNoChannelChat(publicChannelLabel(), emptyText, sizeof(emptyText));
    updated |= setReadonlyTextNewest(taPublicChat, publicChatLog[0] ? publicChatLog : emptyText, force);
  }
  if (currentPage == pagePrivateChat && taFamilyChat) {
    formatNoChannelChat(privateChannelLabel(), emptyText, sizeof(emptyText));
    updated |= setReadonlyTextNewest(taFamilyChat, familyChatLog[0] ? familyChatLog : emptyText, force);
  }
  if (currentPage == pageDirectChat && taDirectChat) {
    updated |= setReadonlyTextNewest(taDirectChat, directChatLog[0] ? directChatLog : "No direct messages yet", force);
  }
  if (updated || force) clearUiDirty(UI_DIRTY_CHAT);
}

static TxRecord* latestTxRecord() {
  if (txHistoryCount == 0) return nullptr;
  size_t index = txHistoryNext == 0 ? TX_HISTORY_COUNT - 1 : txHistoryNext - 1;
  return &txHistory[index];
}

static void describeTxDestination(const TxRecord& record, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (record.direct) {
    snprintf(out, outSize, "!%08lX", (unsigned long)record.to);
  } else {
    snprintf(out, outSize, "%s", channelName(record.channel));
  }
}

static void recordTxMessage(uint8_t channel, uint32_t to, const char* text, size_t len, bool ok, uint32_t packetId) {
  TxRecord& record = txHistory[txHistoryNext];
  memset(&record, 0, sizeof(record));
  size_t copyLen = min(len, sizeof(record.text) - 1);
  if (text && copyLen > 0) memcpy(record.text, text, copyLen);
  record.text[copyLen] = '\0';
  record.to = to;
  record.channel = channel;
  record.sentMs = millis();
  record.direct = isDirectAddress(to);
  record.ok = ok;
  record.packetId = packetId;
  record.delivery = ok ? FieldStatus::Delivery::Queued : FieldStatus::Delivery::Failed;
  record.echoSeen = false;
  txHistoryNext = (txHistoryNext + 1) % TX_HISTORY_COUNT;
  if (txHistoryCount < TX_HISTORY_COUNT) txHistoryCount++;
  markUiDirty(UI_DIRTY_TX | UI_DIRTY_DASHBOARD);
  refreshTxHistoryView();
}

static void markTxEchoSeen(uint8_t channel, uint32_t to, const uint8_t* text, size_t len) {
  for (size_t offset = 0; offset < txHistoryCount; offset++) {
    size_t index = (txHistoryNext + TX_HISTORY_COUNT - 1 - offset) % TX_HISTORY_COUNT;
    TxRecord& record = txHistory[index];
    if (record.channel != channel || record.to != to) continue;
    if (strlen(record.text) != len) continue;
    if (memcmp(record.text, text, len) != 0) continue;
    record.echoSeen = true;
    markUiDirty(UI_DIRTY_TX | UI_DIRTY_DASHBOARD);
    refreshTxHistoryView();
    return;
  }
}

static TxRecord* findTxRecord(uint32_t packetId) {
  if (!packetId) return nullptr;
  for (size_t i = 0; i < txHistoryCount; ++i) {
    size_t index = (txHistoryNext + TX_HISTORY_COUNT - 1 - i) % TX_HISTORY_COUNT;
    if (txHistory[index].packetId == packetId) return &txHistory[index];
  }
  return nullptr;
}

static const char* txStatusText(const TxRecord& record, uint32_t nowMs) {
  switch (FieldStatus::visibleDelivery(record.delivery, record.sentMs, nowMs)) {
    case FieldStatus::Delivery::Queued: return "queued";
    case FieldStatus::Delivery::Sent: return "sent to radio";
    case FieldStatus::Delivery::Acknowledged: return record.direct ? "acknowledged" : "mesh acknowledged";
    case FieldStatus::Delivery::Failed: return "failed";
    default: return "unconfirmed";
  }
}

static const char* txErrorText(const TxRecord& record) {
  if (!record.ok) return "local send failed";
  if (record.queueError) return "radio queue rejected";
  switch (record.errorCode) {
    case 0: return "";
    case 1: return "no route";
    case 2: return "negative acknowledgment";
    case 3: return "radio timeout";
    case 4: return "no radio interface";
    case 5: return "retries exhausted";
    case 6: return "no channel";
    case 7: return "message too large";
    case 8: return "no response";
    case 9: return "duty cycle limit";
    case 33: return "not authorized";
    case 34: return "encryption failed";
    case 35: return "public key unknown";
    default: return "routing error";
  }
}

static void refreshTxHistoryView() {
  if (!taTxHistory || currentPage != pageTxHistory) return;
  if (btnTxRetry) {
    const TxRecord* latest = latestTxRecord();
    bool allowed = latest && !latest->retriedAs && FieldStatus::retryAllowed(latest->delivery, latest->sentMs, millis());
    if (allowed) lv_obj_clear_state(btnTxRetry, LV_STATE_DISABLED);
    else lv_obj_add_state(btnTxRetry, LV_STATE_DISABLED);
  }
  if (txHistoryCount == 0) {
    setReadonlyTextStable(taTxHistory, "No messages sent yet");
    clearUiDirty(UI_DIRTY_TX);
    return;
  }
  uint32_t now = millis();
  static char text[3600];
  size_t used = 0;
  for (size_t offset = 0; offset < txHistoryCount && used < sizeof(text) - 1; offset++) {
    size_t index = (txHistoryNext + TX_HISTORY_COUNT - 1 - offset) % TX_HISTORY_COUNT;
    TxRecord& record = txHistory[index];
    char dest[24];
    describeTxDestination(record, dest, sizeof(dest));
    int written = snprintf(text + used,
                           sizeof(text) - used,
                           "%lus  %s  ch%u %s\n%.233s\n%s%s\n\n",
                           (unsigned long)((now - record.sentMs) / 1000),
                           txStatusText(record, now),
                           record.channel,
                           dest,
                           record.text,
                           txErrorText(record), record.echoSeen ? " (local echo)" : "");
    if (written <= 0) break;
    used += min((size_t)written, sizeof(text) - used - 1);
  }
  text[used] = '\0';
  setReadonlyTextStable(taTxHistory, text);
  clearUiDirty(UI_DIRTY_TX);
}

static bool retryTx(uint32_t packetId) {
  TxRecord* record = findTxRecord(packetId);
  if (!record || !record->text[0] || record->retriedAs ||
      !FieldStatus::retryAllowed(record->delivery, record->sentMs, millis())) return false;
  // Copy before sending: the ring may overwrite the selected record.
  TxRecord copy = *record;
  bool ok = copy.direct ? sendDirectTextMessage(copy.text, copy.to) : sendTextMessage(copy.text, copy.channel);
  if (ok) {
    if (TxRecord* original = findTxRecord(packetId)) original->retriedAs = latestTxRecord()->packetId;
  }
  appendLine(eventLog, LOG_SIZE, ok ? "[local] retried TX\n" : "[local] retry failed\n");
  return ok;
}

static bool retryLastTx() {
  TxRecord* record = latestTxRecord();
  bool ok = record && retryTx(record->packetId);
  if (!ok) appendLine(eventLog, LOG_SIZE, "[local] retry unavailable: wait for result, or no failed/unconfirmed TX\n");
  refreshTxHistoryView();
  return ok;
}

static void formatNodeLabel(const NodeRecord& node, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  const char* label = node.name[0] ? node.name : nullptr;
  if (label && label[0]) {
    snprintf(out, outSize, "%.24s !%08lX", label, (unsigned long)node.num);
  } else {
    snprintf(out, outSize, "!%08lX", (unsigned long)node.num);
  }
}

static int messageDiagIndexForNode(uint32_t nodeNum) {
  for (size_t i = 0; i < msgDiagTargetCount; i++) {
    if (msgDiagTargetNodes[i] == nodeNum) return (int)i;
  }
  return -1;
}

static uint32_t messageDiagNodeForSelection(lv_obj_t* dropdown) {
  if (!dropdown || msgDiagTargetCount == 0) return 0;
  uint16_t selected = lv_dropdown_get_selected(dropdown);
  if (selected >= msgDiagTargetCount) selected = 0;
  return msgDiagTargetNodes[selected];
}

static TxRecord* findMessageDiagTxRecord() {
  if (!msgDiagLastTarget || txHistoryCount == 0) return nullptr;
  for (size_t offset = 0; offset < txHistoryCount; offset++) {
    size_t index = (txHistoryNext + TX_HISTORY_COUNT - 1 - offset) % TX_HISTORY_COUNT;
    TxRecord& record = txHistory[index];
    if (!record.direct || record.to != msgDiagLastTarget) continue;
    if (msgDiagLastSentMs && record.sentMs + 1000 < msgDiagLastSentMs) continue;
    if (msgDiagLastMessage[0] && strncmp(record.text, msgDiagLastMessage, sizeof(record.text)) != 0) continue;
    return &record;
  }
  return nullptr;
}

static const char* messageDiagEchoLabel(const TxRecord* record, uint32_t nowMs) {
  if (!msgDiagLastTarget) return "none";
  if (!msgDiagLastSendOk) return "send failed";
  if (!record) return "not tracked";
  if (record->echoSeen) return "echo seen";
  if (nowMs - record->sentMs > 15000) return "no echo";
  return "waiting";
}

static void formatMessageDiagReadout(char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (!msgDiagLastTarget) {
    strlcpy(out, "Last test\nNo test sent yet.", outSize);
    return;
  }

  uint32_t now = millis();
  uint32_t sentAgeSeconds = msgDiagLastSentMs ? (now - msgDiagLastSentMs) / 1000 : 0;
  char sentAge[8];
  formatShortAge(sentAgeSeconds, sentAge, sizeof(sentAge));
  TxRecord* tx = findMessageDiagTxRecord();
  NodeRecord* node = findNode(msgDiagLastTarget);

  char nodeAge[8] = "--";
  char rangeText[32] = "range --";
  float snr = 0.0f;
  long rssi = 0;
  uint8_t hops = 0;
  const char* gpsText = "no gps";
  if (node) {
    formatShortAge(nodeAgeSeconds(*node, now), nodeAge, sizeof(nodeAge));
    formatRangeBearing(*node, rangeText, sizeof(rangeText));
    snr = node->snr;
    rssi = node->rssi;
    hops = node->hopsAway;
    gpsText = node->hasPosition ? "gps" : "no gps";
  }

  snprintf(out, outSize,
           "Last test\n"
           "To: %s !%08lX\n"
           "TX: %s, %s ago, %u bytes\n"
           "Echo: %s\n"
           "Target: heard %s | SNR %.1f | RSSI %ld | H%u\n"
           "GPS: %s | %s",
           msgDiagLastTargetName[0] ? msgDiagLastTargetName : "node",
           (unsigned long)msgDiagLastTarget,
           msgDiagLastSendOk ? "radio accepted" : "send failed",
           sentAge,
           (unsigned)strlen(msgDiagLastMessage),
           messageDiagEchoLabel(tx, now),
           node ? nodeAge : "--",
           snr,
           rssi,
           hops,
           gpsText,
           node ? rangeText : "range --");
}

static void syncMessageDiagTargetsFromDropdowns() {
  msgDiagTargetA = messageDiagNodeForSelection(dropdownMsgDiagA);
  msgDiagTargetB = messageDiagNodeForSelection(dropdownMsgDiagB);
  if (msgDiagTargetCount > 1 && msgDiagTargetA == msgDiagTargetB) {
    int aIndex = messageDiagIndexForNode(msgDiagTargetA);
    if (aIndex < 0) aIndex = 0;
    int bIndex = (aIndex + 1) % (int)msgDiagTargetCount;
    msgDiagTargetB = msgDiagTargetNodes[bIndex];
    if (dropdownMsgDiagB) lv_dropdown_set_selected(dropdownMsgDiagB, (uint16_t)bIndex);
  }
}

static void refreshMessageDiagView() {
  char options[3072];
  options[0] = '\0';
  uint32_t signature = 2166136261UL;
  size_t favoriteCount = 0;
  size_t targetCount = 0;
  for (size_t i = 0; i < nodeCount && favoriteCount < MAX_NODES; i++) {
    if (!nodes[i].isFavorite) continue;
    msgDiagFavoriteNodes[favoriteCount++] = nodes[i].num;
  }
  bool useFavoritesOnly = favoriteCount >= 2;
  if (useFavoritesOnly) {
    for (size_t i = 0; i < favoriteCount && targetCount < MAX_NODES; i++) {
      msgDiagTargetNodes[targetCount++] = msgDiagFavoriteNodes[i];
    }
  } else {
    for (size_t i = 0; i < nodeCount && targetCount < MAX_NODES; i++) {
      msgDiagTargetNodes[targetCount++] = nodes[i].num;
    }
  }
  for (size_t i = 0; i < targetCount; i++) {
    NodeRecord* node = findNode(msgDiagTargetNodes[i]);
    if (!node) continue;
    signature ^= node->num;
    signature *= 16777619UL;
    for (const char* p = node->name; p && *p; ++p) {
      signature ^= (uint8_t)*p;
      signature *= 16777619UL;
    }
    signature ^= node->isFavorite ? 0xA5A5A5A5UL : 0x5A5A5A5AUL;
    signature *= 16777619UL;
    char line[80];
    if (node->isFavorite) {
      snprintf(line, sizeof(line), "FAV  %.24s !%08lX",
               node->name[0] ? node->name : nodeName(node->num),
               (unsigned long)node->num);
    } else {
      formatNodeLabel(*node, line, sizeof(line));
    }
    size_t used = strlen(options);
    if (used > 0 && used < sizeof(options) - 1) {
      options[used++] = '\n';
      options[used] = '\0';
    }
    snprintf(options + strlen(options), sizeof(options) - strlen(options), "%s", line);
  }
  msgDiagFavoriteCount = favoriteCount;
  msgDiagTargetCount = targetCount;
  msgDiagTargetUsesAllNodes = !useFavoritesOnly && targetCount > 0;
  bool targetsChanged = signature != msgDiagTargetSignature;
  msgDiagTargetSignature = signature;
  if (msgDiagTargetCount == 0) {
    strlcpy(options, "No nodes loaded", sizeof(options));
    msgDiagTargetA = 0;
    msgDiagTargetB = 0;
  }

  if (targetsChanged || !dropdownMsgDiagA || !dropdownMsgDiagB || msgDiagTargetCount == 0) {
    if (msgDiagTargetCount > 0) {
    int aIndex = messageDiagIndexForNode(msgDiagTargetA);
    if (aIndex < 0) aIndex = 0;
    int bIndex = messageDiagIndexForNode(msgDiagTargetB);
    if (bIndex < 0 || (msgDiagTargetCount > 1 && bIndex == aIndex)) {
      bIndex = msgDiagTargetCount > 1 ? ((aIndex + 1) % (int)msgDiagTargetCount) : aIndex;
    }
    msgDiagTargetA = msgDiagTargetNodes[aIndex];
    msgDiagTargetB = msgDiagTargetNodes[bIndex];
      if (dropdownMsgDiagA) lv_dropdown_set_selected(dropdownMsgDiagA, (uint16_t)aIndex);
      if (dropdownMsgDiagB) lv_dropdown_set_selected(dropdownMsgDiagB, (uint16_t)bIndex);
    }
    if (dropdownMsgDiagA) lv_dropdown_set_options(dropdownMsgDiagA, options);
    if (dropdownMsgDiagB) lv_dropdown_set_options(dropdownMsgDiagB, options);
  }

  char aLabel[48];
  char bLabel[48];
  NodeRecord* aNode = msgDiagTargetA ? findNode(msgDiagTargetA) : nullptr;
  NodeRecord* bNode = msgDiagTargetB ? findNode(msgDiagTargetB) : nullptr;
  if (aNode) formatNodeLabel(*aNode, aLabel, sizeof(aLabel));
  else strlcpy(aLabel, "none", sizeof(aLabel));
  if (bNode) formatNodeLabel(*bNode, bLabel, sizeof(bLabel));
  else strlcpy(bLabel, "none", sizeof(bLabel));

  char txText[300];
  formatMessageDiagReadout(txText, sizeof(txText));

  if (msgDiagLastTarget) {
    snprintf(msgDiagStatus, sizeof(msgDiagStatus),
             "Last: %s  Pair: %s%s",
             messageDiagEchoLabel(findMessageDiagTxRecord(), millis()),
             msgDiagTargetCount >= 2 ? "ready" : "not ready",
             msgDiagTargetUsesAllNodes ? "  all nodes fallback" : "");
  } else {
    snprintf(msgDiagStatus, sizeof(msgDiagStatus),
             "Favorites: %u  Targets: %u  Pair: %s%s",
             (unsigned)msgDiagFavoriteCount,
             (unsigned)msgDiagTargetCount,
             msgDiagTargetCount >= 2 ? "ready" : "not ready",
             msgDiagTargetUsesAllNodes ? "  all nodes fallback" : "");
  }
  snprintf(msgDiagDetail, sizeof(msgDiagDetail),
           "A: %s\n"
           "B: %s\n\n"
           "%s",
           aLabel,
           bLabel,
           txText);

  if (lblMsgDiagStatus) setReadoutText(lblMsgDiagStatus, msgDiagStatus);
  if (lblMsgDiagDetail) setReadoutText(lblMsgDiagDetail, msgDiagDetail);
}

static bool sendMessageDiagTo(uint32_t nodeNum, const char* targetName) {
  if (!isDirectAddress(nodeNum)) {
    strlcpy(msgDiagStatus, "Select a valid favorite node.", sizeof(msgDiagStatus));
    strlcpy(msgDiagDetail, "No direct target selected.", sizeof(msgDiagDetail));
    if (lblMsgDiagStatus) setReadoutText(lblMsgDiagStatus, msgDiagStatus);
    if (lblMsgDiagDetail) setReadoutText(lblMsgDiagDetail, msgDiagDetail);
    return false;
  }
  const char* text = taMsgDiagMessage ? lv_textarea_get_text(taMsgDiagMessage) : nullptr;
  if (!text || !text[0]) text = "Range test";
  msgDiagLastTarget = nodeNum;
  msgDiagLastSentMs = millis();
  msgDiagLastSendOk = false;
  strlcpy(msgDiagLastTargetName, targetName && targetName[0] ? targetName : "node", sizeof(msgDiagLastTargetName));
  strlcpy(msgDiagLastMessage, text, sizeof(msgDiagLastMessage));
  bool ok = sendDirectTextMessage(text, nodeNum);
  msgDiagLastSendOk = ok;
  if (ok) {
    snprintf(msgDiagStatus, sizeof(msgDiagStatus), "Sent to %s", targetName && targetName[0] ? targetName : "favorite");
  } else {
    snprintf(msgDiagStatus, sizeof(msgDiagStatus), "Send failed to %s", targetName && targetName[0] ? targetName : "favorite");
  }
  formatMessageDiagReadout(msgDiagDetail, sizeof(msgDiagDetail));
  if (lblMsgDiagStatus) setReadoutText(lblMsgDiagStatus, msgDiagStatus);
  if (lblMsgDiagDetail) setReadoutText(lblMsgDiagDetail, msgDiagDetail);
  return ok;
}

static void messageDiagDropdownEvent(lv_event_t* e) {
  (void)e;
  syncMessageDiagTargetsFromDropdowns();
  refreshMessageDiagView();
}

static void messageDiagSendAEvent(lv_event_t*) {
  syncMessageDiagTargetsFromDropdowns();
  char label[48];
  if (NodeRecord* node = findNode(msgDiagTargetA)) formatNodeLabel(*node, label, sizeof(label));
  else strlcpy(label, "A", sizeof(label));
  sendMessageDiagTo(msgDiagTargetA, label);
}

static void messageDiagSendBEvent(lv_event_t*) {
  syncMessageDiagTargetsFromDropdowns();
  char label[48];
  if (NodeRecord* node = findNode(msgDiagTargetB)) formatNodeLabel(*node, label, sizeof(label));
  else strlcpy(label, "B", sizeof(label));
  sendMessageDiagTo(msgDiagTargetB, label);
}

static void messageDiagSwapEvent(lv_event_t*) {
  uint32_t tmp = msgDiagTargetA;
  msgDiagTargetA = msgDiagTargetB;
  msgDiagTargetB = tmp;
  int aIndex = messageDiagIndexForNode(msgDiagTargetA);
  int bIndex = messageDiagIndexForNode(msgDiagTargetB);
  if (dropdownMsgDiagA && aIndex >= 0) lv_dropdown_set_selected(dropdownMsgDiagA, (uint16_t)aIndex);
  if (dropdownMsgDiagB && bIndex >= 0) lv_dropdown_set_selected(dropdownMsgDiagB, (uint16_t)bIndex);
  refreshMessageDiagView();
}

static void rememberLocalSentText(uint8_t channel, uint32_t to, const char* text, size_t len) {
  size_t copyLen = min(len, sizeof(lastLocalSentText) - 1);
  memcpy(lastLocalSentText, text, copyLen);
  lastLocalSentText[copyLen] = '\0';
  lastLocalSentChannel = channel;
  lastLocalSentTo = to;
  lastLocalSentMs = millis();
}

static bool isRecentLocalEcho(uint8_t channel, uint32_t to, const uint8_t* text, size_t len) {
  if (!lastLocalSentMs || channel != lastLocalSentChannel || to != lastLocalSentTo) return false;
  if (millis() - lastLocalSentMs > 15000) return false;
  if (len != strlen(lastLocalSentText)) return false;
  return memcmp(text, lastLocalSentText, len) == 0;
}

static void formatChatTimestamp(char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (localGps.date.isValid() && localGps.time.isValid()) {
    snprintf(out, outSize, "%04d-%02d-%02d %02d:%02d:%02dZ",
             localGps.date.year(),
             localGps.date.month(),
             localGps.date.day(),
             localGps.time.hour(),
             localGps.time.minute(),
             localGps.time.second());
    return;
  }

  uint32_t seconds = millis() / 1000;
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds / 60) % 60;
  seconds %= 60;
  snprintf(out, outSize, "up %lu:%02lu:%02lu",
           (unsigned long)hours,
           (unsigned long)minutes,
           (unsigned long)seconds);
}

static bool isDirectAddress(uint32_t to) {
  return to != 0 && to != BROADCAST_ADDR;
}

static void copyChatPreview(char* out, size_t outSize, const char* sender, const uint8_t* text, size_t len) {
  if (!out || outSize == 0) return;
  size_t pos = 0;
  if (sender && sender[0] && strcmp(sender, "me") != 0) {
    pos = snprintf(out, outSize, "%s: ", sender);
    if (pos >= outSize) {
      out[outSize - 1] = '\0';
      return;
    }
  }
  for (size_t i = 0; i < len && pos + 1 < outSize; i++) {
    char c = (char)text[i];
    out[pos++] = (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
  }
  out[pos] = '\0';
}

static void recordChatActivity(uint8_t channel, uint32_t to, const char* sender, const uint8_t* text, size_t len) {
  copyChatPreview(previewLatest, sizeof(previewLatest), sender, text, len);
  bool isLocal = sender && strcmp(sender, "me") == 0;
  if (isDirectAddress(to)) {
    copyChatPreview(previewDirect, sizeof(previewDirect), sender, text, len);
    if (!isLocal && currentPage != pageDirectChat && unreadDirect < 999) unreadDirect++;
  } else if (isPrivateChannel(channel)) {
    copyChatPreview(previewFamily, sizeof(previewFamily), sender, text, len);
    if (!isLocal && currentPage != pagePrivateChat && unreadFamily < 999) unreadFamily++;
  } else {
    copyChatPreview(previewPublic, sizeof(previewPublic), sender, text, len);
    if (!isLocal && currentPage != pagePublicChat && unreadPublic < 999) unreadPublic++;
  }
  refreshDashboardLabels();
}

static void appendChatMessage(uint8_t channel, uint32_t to, const char* sender, const uint8_t* text, size_t len) {
  if (!text || len == 0) return;
  char stamp[28];
  formatChatTimestamp(stamp, sizeof(stamp));
  char line[320];
  snprintf(line, sizeof(line), "[%s] [%s] %.*s\n", stamp, sender && sender[0] ? sender : "unknown", (int)len, text);
  if (isDirectAddress(to)) appendLine(directChatLog, CHAT_SIZE, line);
  else appendLine(isPrivateChannel(channel) ? familyChatLog : publicChatLog, CHAT_SIZE, line);
  recordChatActivity(channel, to, sender, text, len);
  refreshChatViews();
}

static bool writeStreamFrame(const uint8_t* payload, size_t len) {
  const uint8_t header[] = {0x94, 0xC3, (uint8_t)(len >> 8), (uint8_t)len};
  size_t written = SerialLoRa.write(header, sizeof(header));
  bytesToRadio += written;
  if (written != sizeof(header)) return false;
  written = SerialLoRa.write(payload, len);
  bytesToRadio += written;
  return written == len;
}

static uint32_t allocateMeshPacketId() {
  uint32_t id = nextMeshPacketId++;
  if (id == 0) id = nextMeshPacketId++;
  if (nextMeshPacketId == 0) nextMeshPacketId = 1;
  return id;
}

static uint8_t activeHopLimit() {
  if (heltecConfig.hasLora && heltecConfig.lora.hop_limit > 0) {
    return min<uint8_t>(heltecConfig.lora.hop_limit, 7);
  }
  return 3;
}

static void applyClientPacketDefaults(meshtastic_MeshPacket& packet) {
  if (packet.id == 0) packet.id = allocateMeshPacketId();
  if (packet.hop_limit == 0) packet.hop_limit = activeHopLimit();
  if (packet.priority == meshtastic_MeshPacket_Priority_UNSET) {
    packet.priority = meshtastic_MeshPacket_Priority_RELIABLE;
  }
}

static bool sendConfigRequest() {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_want_config_id_tag;
  toRadio.want_config_id = nextConfigRequestId++;
  if (nextConfigRequestId == 0) nextConfigRequestId = 1;

  uint8_t out[64];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  if (!writeStreamFrame(out, stream.bytes_written)) return false;
  lastConfigRequestMs = millis();
  if (configRequestCount < 255) configRequestCount++;
  appendLine(eventLog, LOG_SIZE, "[local] requested Heltec config\n");
  return true;
}

static bool sendRadioHeartbeat() {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_heartbeat_tag;
  toRadio.heartbeat.nonce = ++heartbeatNonce;
  if (heartbeatNonce == 0) toRadio.heartbeat.nonce = ++heartbeatNonce;

  uint8_t out[64];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  writeStreamFrame(out, stream.bytes_written);
  lastHeartbeatMs = millis();
  heartbeatCount++;
  return true;
}

static void serviceRadioApiKeepalive() {
  uint32_t now = millis();
  if (lastHeartbeatMs == 0 || now - lastHeartbeatMs >= RADIO_API_HEARTBEAT_MS) {
    sendRadioHeartbeat();
  }

  if (lastByteMs == 0) return;
  if (now - lastByteMs < RADIO_API_IDLE_RESYNC_MS) return;
  if (lastApiResyncMs != 0 && now - lastApiResyncMs < RADIO_API_RESYNC_COOLDOWN_MS) return;

  lastApiResyncMs = now;
  apiResyncCount++;
  appendLine(eventLog, LOG_SIZE, "[link] serial API idle; refreshing Heltec stream\n");
  sendConfigRequest();
}

static void serviceConfigRequests() {
  bool needsInitialData = stats.myNodeNum == 0 || nodeInfoPackets == 0;
  bool needsChannelData = privateChannelIndex < 0 && configFrames == 0;
  if (!(needsInitialData || needsChannelData)) return;
  if (configRequestCount >= 12) return;
  if (millis() - lastConfigRequestMs < 30000) return;
  sendConfigRequest();
}

static bool sendTextMessage(const char* text, int8_t channelIndex) {
  if (!text || !text[0] || strlen(text) > 233) return false;
  ChannelRecord* selected = channelIndex >= 0 && channelIndex < (int8_t)MAX_CHANNELS
                            ? channelRecordByIndex(channelIndex) : nullptr;
  if (!selected || !selected->enabled) {
    appendLine(eventLog, LOG_SIZE, "[local] send rejected: channel is disabled or not loaded\n");
    return false;
  }

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = BROADCAST_ADDR;
  packet.channel = channelIndex;
  packet.want_ack = true;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
  applyClientPacketDefaults(packet);

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false, packet.id);
    return false;
  }
  if (!writeStreamFrame(out, stream.bytes_written)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false, packet.id);
    return false;
  }

  rememberLocalSentText(packet.channel, packet.to, text, data.payload.size);
  recordTxMessage(packet.channel, packet.to, text, data.payload.size, true, packet.id);
  appendChatMessage(packet.channel, packet.to, "me", data.payload.bytes, data.payload.size);

  char line[96];
  snprintf(line, sizeof(line), "[local] text sent on %s\n", channelName(packet.channel));
  appendLine(eventLog, LOG_SIZE, line);
  return true;
}

static bool sendDirectTextMessage(const char* text, uint32_t toNode) {
  if (!text || !text[0] || strlen(text) > 233 || !isDirectAddress(toNode)) return false;

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = toNode;
  packet.channel = PUBLIC_CHANNEL_INDEX;
  packet.want_ack = true;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
  applyClientPacketDefaults(packet);

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false, packet.id);
    return false;
  }
  if (!writeStreamFrame(out, stream.bytes_written)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false, packet.id);
    return false;
  }

  rememberLocalSentText(packet.channel, packet.to, text, data.payload.size);
  recordTxMessage(packet.channel, packet.to, text, data.payload.size, true, packet.id);
  appendChatMessage(packet.channel, packet.to, "me", data.payload.bytes, data.payload.size);

  char line[96];
  snprintf(line, sizeof(line), "[local] direct text sent to !%08lX\n", (unsigned long)toNode);
  appendLine(eventLog, LOG_SIZE, line);
  return true;
}

static bool sendLocalAdmin(const meshtastic_AdminMessage& admin) {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = stats.myNodeNum ? stats.myNodeNum : BROADCAST_ADDR;
  packet.channel = PUBLIC_CHANNEL_INDEX;
  packet.want_ack = false;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_ADMIN_APP;
  data.want_response = true;

  pb_ostream_t adminStream = pb_ostream_from_buffer(data.payload.bytes, sizeof(data.payload.bytes));
  if (!pb_encode(&adminStream, meshtastic_AdminMessage_fields, &admin)) return false;
  data.payload.size = adminStream.bytes_written;

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  return writeStreamFrame(out, stream.bytes_written);
}

static bool sendHeltecReboot(uint8_t seconds) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_reboot_seconds_tag;
  admin.reboot_seconds = seconds;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec reboot requested\n" : "[local] Heltec reboot request failed\n");
  return ok;
}

static bool sendHeltecNodeDbReset() {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_nodedb_reset_tag;
  admin.nodedb_reset = true;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec NodeDB reset sent\n" : "[local] Heltec NodeDB reset failed\n");
  return ok;
}

static bool sendHeltecFavoriteNode(uint32_t nodeNum, bool favorite) {
  if (!isDirectAddress(nodeNum)) return false;
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = favorite
    ? meshtastic_AdminMessage_set_favorite_node_tag
    : meshtastic_AdminMessage_remove_favorite_node_tag;
  if (favorite) admin.set_favorite_node = nodeNum;
  else admin.remove_favorite_node = nodeNum;
  bool ok = sendLocalAdmin(admin);
  char line[128];
  snprintf(line, sizeof(line), "[local] %s favorite !%08lX %s\n",
           favorite ? "set" : "cleared",
           (unsigned long)nodeNum,
           ok ? "sent" : "failed");
  appendLine(eventLog, LOG_SIZE, line);
  return ok;
}

static meshtastic_Config_LoRaConfig_RegionCode parseRegion(const String& value) {
  if (value == "US") return meshtastic_Config_LoRaConfig_RegionCode_US;
  if (value == "EU_868") return meshtastic_Config_LoRaConfig_RegionCode_EU_868;
  if (value == "CN") return meshtastic_Config_LoRaConfig_RegionCode_CN;
  if (value == "JP") return meshtastic_Config_LoRaConfig_RegionCode_JP;
  if (value == "ANZ") return meshtastic_Config_LoRaConfig_RegionCode_ANZ;
  if (value == "KR") return meshtastic_Config_LoRaConfig_RegionCode_KR;
  if (value == "TW") return meshtastic_Config_LoRaConfig_RegionCode_TW;
  if (value == "RU") return meshtastic_Config_LoRaConfig_RegionCode_RU;
  if (value == "IN") return meshtastic_Config_LoRaConfig_RegionCode_IN;
  return meshtastic_Config_LoRaConfig_RegionCode_UNSET;
}

static meshtastic_Config_LoRaConfig_ModemPreset parseModemPreset(const String& value) {
  if (value == "LONG_SLOW") return meshtastic_Config_LoRaConfig_ModemPreset_LONG_SLOW;
  if (value == "MEDIUM_FAST") return meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_FAST;
  if (value == "MEDIUM_SLOW") return meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_SLOW;
  if (value == "SHORT_FAST") return meshtastic_Config_LoRaConfig_ModemPreset_SHORT_FAST;
  if (value == "SHORT_SLOW") return meshtastic_Config_LoRaConfig_ModemPreset_SHORT_SLOW;
  return meshtastic_Config_LoRaConfig_ModemPreset_LONG_FAST;
}

static meshtastic_Channel_Role parseChannelRole(const String& value) {
  if (value == "PRIMARY") return meshtastic_Channel_Role_PRIMARY;
  if (value == "SECONDARY") return meshtastic_Channel_Role_SECONDARY;
  return meshtastic_Channel_Role_DISABLED;
}

static meshtastic_ModuleConfig_SerialConfig_Serial_Baud parseSerialBaud(const String& value) {
  if (value == "9600") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_9600;
  if (value == "19200") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_19200;
  if (value == "38400") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_38400;
  if (value == "57600") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_57600;
  if (value == "230400") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_230400;
  if (value == "460800") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_460800;
  if (value == "576000") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_576000;
  if (value == "921600") return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_921600;
  return meshtastic_ModuleConfig_SerialConfig_Serial_Baud_BAUD_115200;
}

static meshtastic_ModuleConfig_SerialConfig_Serial_Mode parseSerialMode(const String& value) {
  if (value == "SIMPLE") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_SIMPLE;
  if (value == "TEXTMSG") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_TEXTMSG;
  if (value == "NMEA") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_NMEA;
  if (value == "CALTOPO") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_CALTOPO;
  if (value == "WS85") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_WS85;
  if (value == "VE_DIRECT") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_VE_DIRECT;
  if (value == "MS_CONFIG") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_MS_CONFIG;
  if (value == "LOG") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_LOG;
  if (value == "LOGTEXT") return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_LOGTEXT;
  return meshtastic_ModuleConfig_SerialConfig_Serial_Mode_PROTO;
}

static meshtastic_Config_DeviceConfig_Role parseDeviceRole(const String& value) {
  if (value == "CLIENT_MUTE") return meshtastic_Config_DeviceConfig_Role_CLIENT_MUTE;
  if (value == "ROUTER") return meshtastic_Config_DeviceConfig_Role_ROUTER;
  if (value == "ROUTER_CLIENT") return meshtastic_Config_DeviceConfig_Role_ROUTER_CLIENT;
  if (value == "REPEATER") return meshtastic_Config_DeviceConfig_Role_REPEATER;
  if (value == "TRACKER") return meshtastic_Config_DeviceConfig_Role_TRACKER;
  if (value == "SENSOR") return meshtastic_Config_DeviceConfig_Role_SENSOR;
  if (value == "TAK") return meshtastic_Config_DeviceConfig_Role_TAK;
  if (value == "CLIENT_HIDDEN") return meshtastic_Config_DeviceConfig_Role_CLIENT_HIDDEN;
  if (value == "LOST_AND_FOUND") return meshtastic_Config_DeviceConfig_Role_LOST_AND_FOUND;
  if (value == "TAK_TRACKER") return meshtastic_Config_DeviceConfig_Role_TAK_TRACKER;
  if (value == "ROUTER_LATE") return meshtastic_Config_DeviceConfig_Role_ROUTER_LATE;
  if (value == "CLIENT_BASE") return meshtastic_Config_DeviceConfig_Role_CLIENT_BASE;
  return meshtastic_Config_DeviceConfig_Role_CLIENT;
}

static meshtastic_Config_DeviceConfig_RebroadcastMode parseRebroadcastMode(const String& value) {
  if (value == "ALL_SKIP_DECODING") return meshtastic_Config_DeviceConfig_RebroadcastMode_ALL_SKIP_DECODING;
  if (value == "LOCAL_ONLY") return meshtastic_Config_DeviceConfig_RebroadcastMode_LOCAL_ONLY;
  if (value == "KNOWN_ONLY") return meshtastic_Config_DeviceConfig_RebroadcastMode_KNOWN_ONLY;
  if (value == "NONE") return meshtastic_Config_DeviceConfig_RebroadcastMode_NONE;
  if (value == "CORE_PORTNUMS_ONLY") return meshtastic_Config_DeviceConfig_RebroadcastMode_CORE_PORTNUMS_ONLY;
  return meshtastic_Config_DeviceConfig_RebroadcastMode_ALL;
}

static meshtastic_Config_DeviceConfig_BuzzerMode parseBuzzerMode(const String& value) {
  if (value == "DISABLED") return meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED;
  if (value == "NOTIFICATIONS_ONLY") return meshtastic_Config_DeviceConfig_BuzzerMode_NOTIFICATIONS_ONLY;
  if (value == "SYSTEM_ONLY") return meshtastic_Config_DeviceConfig_BuzzerMode_SYSTEM_ONLY;
  if (value == "DIRECT_MSG_ONLY") return meshtastic_Config_DeviceConfig_BuzzerMode_DIRECT_MSG_ONLY;
  return meshtastic_Config_DeviceConfig_BuzzerMode_ALL_ENABLED;
}

static meshtastic_Config_PositionConfig_GpsMode parseGpsMode(const String& value) {
  if (value == "ENABLED") return meshtastic_Config_PositionConfig_GpsMode_ENABLED;
  if (value == "NOT_PRESENT") return meshtastic_Config_PositionConfig_GpsMode_NOT_PRESENT;
  return meshtastic_Config_PositionConfig_GpsMode_DISABLED;
}

static bool formBool(const char* name) {
  return server.arg(name) == "1";
}

static uint32_t formU32(const char* name, uint32_t fallback) {
  if (!server.hasArg(name)) return fallback;
  int64_t value = 0;
  return Reliability::parseInteger(server.arg(name).c_str(), 0, UINT32_MAX, value) ? (uint32_t)value : fallback;
}

static int32_t formI32(const char* name, int32_t fallback) {
  if (!server.hasArg(name)) return fallback;
  int64_t value = 0;
  return Reliability::parseInteger(server.arg(name).c_str(), INT32_MIN, INT32_MAX, value) ? (int32_t)value : fallback;
}

static uint8_t formU8(const char* name, uint8_t fallback) {
  if (!server.hasArg(name)) return fallback;
  int64_t value = 0;
  return Reliability::parseInteger(server.arg(name).c_str(), 0, UINT8_MAX, value) ? (uint8_t)value : fallback;
}

static void formStr(const char* name, char* dest, size_t destSize, bool keepBlank) {
  if (!server.hasArg(name)) return;
  String value = server.arg(name);
  if (!keepBlank && !value.length()) return;
  strlcpy(dest, value.c_str(), destSize);
}

static bool sendHeltecLoraConfig(const String& region, const String& preset, uint8_t hopLimit, int8_t txPower) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_lora_tag;
  admin.set_config.payload_variant.lora.use_preset = true;
  admin.set_config.payload_variant.lora.region = parseRegion(region);
  admin.set_config.payload_variant.lora.modem_preset = parseModemPreset(preset);
  admin.set_config.payload_variant.lora.hop_limit = min<uint8_t>(hopLimit, 7);
  admin.set_config.payload_variant.lora.tx_enabled = true;
  admin.set_config.payload_variant.lora.tx_power = txPower;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec LoRa config sent\n" : "[local] Heltec LoRa config failed\n");
  return ok;
}

static bool sendHeltecSerialConfig(bool enabled, uint32_t rxd, uint32_t txd, const String& baud, const String& mode, uint32_t timeoutMs, bool echo, bool overrideConsole) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_module_config_tag;
  admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_serial_tag;
  meshtastic_ModuleConfig_SerialConfig& serial = admin.set_module_config.payload_variant.serial;
  if (heltecConfig.hasSerial) {
    serial = heltecConfig.serial;
  }
  serial.enabled = enabled;
  serial.echo = echo;
  serial.rxd = rxd;
  serial.txd = txd;
  serial.baud = parseSerialBaud(baud);
  serial.timeout = timeoutMs;
  serial.mode = parseSerialMode(mode);
  serial.override_console_serial_port = overrideConsole;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec serial config sent\n" : "[local] Heltec serial config failed\n");
  return ok;
}

static bool sendHeltecOwnerName(const String& name, const String& shortName) {
  if (!name.length()) return false;
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_owner_tag;
  snprintf(admin.set_owner.id, sizeof(admin.set_owner.id), "!%08lX", (unsigned long)stats.myNodeNum);
  strlcpy(admin.set_owner.long_name, name.c_str(), sizeof(admin.set_owner.long_name));
  String shortValue = shortName.length() ? shortName : name.substring(0, min<size_t>(4, name.length()));
  strlcpy(admin.set_owner.short_name, shortValue.c_str(), sizeof(admin.set_owner.short_name));
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec owner name sent\n" : "[local] Heltec owner name failed\n");
  return ok;
}

static bool sendHeltecDeviceConfig(const String& role, const String& rebroadcast, uint32_t nodeInfoSecs, const String& tz, bool ledOff, const String& buzzer) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_device_tag;
  meshtastic_Config_DeviceConfig& device = admin.set_config.payload_variant.device;
  if (heltecConfig.hasDevice) {
    device = heltecConfig.device;
  }
  if (role.length()) device.role = parseDeviceRole(role);
  if (rebroadcast.length()) device.rebroadcast_mode = parseRebroadcastMode(rebroadcast);
  device.node_info_broadcast_secs = nodeInfoSecs;
  device.led_heartbeat_disabled = ledOff;
  if (buzzer.length()) device.buzzer_mode = parseBuzzerMode(buzzer);
  if (tz.length()) strlcpy(device.tzdef, tz.c_str(), sizeof(device.tzdef));
  bool ok = sendLocalAdmin(admin);
  char line[128];
  snprintf(line, sizeof(line), "[local] Heltec device config %s role=%s rebroadcast=%s\n",
           ok ? "sent" : "failed",
           deviceRoleName(device.role),
           rebroadcastName(device.rebroadcast_mode));
  appendLine(eventLog, LOG_SIZE, line);
  return ok;
}

static bool sendHeltecPositionConfig(bool gpsEnabled, const String& gpsMode, bool fixedPosition, bool smartBroadcast, uint32_t broadcastSecs, uint32_t gpsUpdateSecs, uint32_t gpsAttemptSecs, uint32_t smartMinMeters, uint32_t smartMinSecs) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_position_tag;
  admin.set_config.payload_variant.position.gps_enabled = gpsEnabled;
  admin.set_config.payload_variant.position.gps_mode = parseGpsMode(gpsMode);
  admin.set_config.payload_variant.position.fixed_position = fixedPosition;
  admin.set_config.payload_variant.position.position_broadcast_smart_enabled = smartBroadcast;
  admin.set_config.payload_variant.position.position_broadcast_secs = broadcastSecs;
  admin.set_config.payload_variant.position.gps_update_interval = gpsUpdateSecs;
  admin.set_config.payload_variant.position.gps_attempt_time = gpsAttemptSecs;
  admin.set_config.payload_variant.position.broadcast_smart_minimum_distance = smartMinMeters;
  admin.set_config.payload_variant.position.broadcast_smart_minimum_interval_secs = smartMinSecs;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec position config sent\n" : "[local] Heltec position config failed\n");
  return ok;
}

static bool sendHeltecPowerConfig(bool powerSaving, uint32_t shutdownSecs, uint32_t waitBluetoothSecs, uint32_t sdsSecs, uint32_t lsSecs, uint32_t minWakeSecs) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_power_tag;
  admin.set_config.payload_variant.power.is_power_saving = powerSaving;
  admin.set_config.payload_variant.power.on_battery_shutdown_after_secs = shutdownSecs;
  admin.set_config.payload_variant.power.wait_bluetooth_secs = waitBluetoothSecs;
  admin.set_config.payload_variant.power.sds_secs = sdsSecs;
  admin.set_config.payload_variant.power.ls_secs = lsSecs;
  admin.set_config.payload_variant.power.min_wake_secs = minWakeSecs;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec power config sent\n" : "[local] Heltec power config failed\n");
  return ok;
}

static bool sendHeltecNetworkConfig() {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_network_tag;
  meshtastic_Config_NetworkConfig& cfg = admin.set_config.payload_variant.network;
  if (heltecConfig.hasNetwork) cfg = heltecConfig.network;
  cfg.wifi_enabled = formBool("wifi");
  formStr("ssid", cfg.wifi_ssid, sizeof(cfg.wifi_ssid), true);
  formStr("psk", cfg.wifi_psk, sizeof(cfg.wifi_psk), false);
  formStr("ntp", cfg.ntp_server, sizeof(cfg.ntp_server), true);
  formStr("syslog", cfg.rsyslog_server, sizeof(cfg.rsyslog_server), true);
  cfg.eth_enabled = formBool("eth");
  cfg.ipv6_enabled = formBool("ipv6");
  if (formBool("udp")) cfg.enabled_protocols |= meshtastic_Config_NetworkConfig_ProtocolFlags_UDP_BROADCAST;
  else cfg.enabled_protocols &= ~((uint32_t)meshtastic_Config_NetworkConfig_ProtocolFlags_UDP_BROADCAST);
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec network config sent\n" : "[local] Heltec network config failed\n");
  return ok;
}

static bool sendHeltecDisplayConfig() {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_display_tag;
  meshtastic_Config_DisplayConfig& cfg = admin.set_config.payload_variant.display;
  if (heltecConfig.hasDisplay) cfg = heltecConfig.display;
  cfg.screen_on_secs = formU32("screen", cfg.screen_on_secs);
  cfg.auto_screen_carousel_secs = formU32("carousel", cfg.auto_screen_carousel_secs);
  cfg.compass_north_top = formBool("north");
  cfg.flip_screen = formBool("flip");
  cfg.units = (meshtastic_Config_DisplayConfig_DisplayUnits)formU32("units", cfg.units);
  cfg.oled = (meshtastic_Config_DisplayConfig_OledType)formU32("oled", cfg.oled);
  cfg.displaymode = (meshtastic_Config_DisplayConfig_DisplayMode)formU32("mode", cfg.displaymode);
  cfg.heading_bold = formBool("bold");
  cfg.wake_on_tap_or_motion = formBool("wake");
  cfg.use_12h_clock = formBool("clock12");
  cfg.use_long_node_name = formBool("longName");
  cfg.enable_message_bubbles = formBool("bubbles");
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec display config sent\n" : "[local] Heltec display config failed\n");
  return ok;
}

static bool sendHeltecBluetoothConfig() {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_bluetooth_tag;
  meshtastic_Config_BluetoothConfig& cfg = admin.set_config.payload_variant.bluetooth;
  if (heltecConfig.hasBluetooth) cfg = heltecConfig.bluetooth;
  cfg.enabled = formBool("enabled");
  cfg.mode = (meshtastic_Config_BluetoothConfig_PairingMode)formU32("mode", cfg.mode);
  cfg.fixed_pin = formU32("pin", cfg.fixed_pin);
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec bluetooth config sent\n" : "[local] Heltec bluetooth config failed\n");
  return ok;
}

static bool sendHeltecModuleConfig(const String& section) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_module_config_tag;

  if (section == "mqtt") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_mqtt_tag;
    meshtastic_ModuleConfig_MQTTConfig& cfg = admin.set_module_config.payload_variant.mqtt;
    if (heltecConfig.hasMqtt) cfg = heltecConfig.mqtt;
    cfg.enabled = formBool("enabled");
    formStr("address", cfg.address, sizeof(cfg.address), true);
    formStr("username", cfg.username, sizeof(cfg.username), true);
    formStr("password", cfg.password, sizeof(cfg.password), false);
    formStr("root", cfg.root, sizeof(cfg.root), true);
    cfg.encryption_enabled = formBool("encryption");
    cfg.json_enabled = formBool("json");
    cfg.tls_enabled = formBool("tls");
    cfg.proxy_to_client_enabled = formBool("proxy");
    cfg.map_reporting_enabled = formBool("mapReporting");
  } else if (section == "external_notification") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_external_notification_tag;
    meshtastic_ModuleConfig_ExternalNotificationConfig& cfg = admin.set_module_config.payload_variant.external_notification;
    if (heltecConfig.hasExternalNotification) cfg = heltecConfig.externalNotification;
    cfg.enabled = formBool("enabled");
    cfg.output = formU32("output", cfg.output);
    cfg.output_vibra = formU8("vibra", cfg.output_vibra);
    cfg.output_buzzer = formU8("buzzer", cfg.output_buzzer);
    cfg.output_ms = formU32("outputMs", cfg.output_ms);
    cfg.nag_timeout = (uint16_t)min<uint32_t>(formU32("nag", cfg.nag_timeout), 65535);
    cfg.active = formBool("active");
    cfg.alert_message = formBool("alertMessage");
    cfg.alert_message_vibra = formBool("alertMessageVibra");
    cfg.alert_message_buzzer = formBool("alertMessageBuzzer");
    cfg.alert_bell = formBool("alertBell");
    cfg.alert_bell_vibra = formBool("alertBellVibra");
    cfg.alert_bell_buzzer = formBool("alertBellBuzzer");
    cfg.use_pwm = formBool("usePwm");
    cfg.use_i2s_as_buzzer = formBool("useI2s");
  } else if (section == "store_forward") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_store_forward_tag;
    meshtastic_ModuleConfig_StoreForwardConfig& cfg = admin.set_module_config.payload_variant.store_forward;
    if (heltecConfig.hasStoreForward) cfg = heltecConfig.storeForward;
    cfg.enabled = formBool("enabled");
    cfg.heartbeat = formBool("heartbeat");
    cfg.records = formU32("records", cfg.records);
    cfg.history_return_max = formU32("historyMax", cfg.history_return_max);
    cfg.history_return_window = formU32("historyWindow", cfg.history_return_window);
    cfg.is_server = formBool("server");
  } else if (section == "range_test") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_range_test_tag;
    meshtastic_ModuleConfig_RangeTestConfig& cfg = admin.set_module_config.payload_variant.range_test;
    if (heltecConfig.hasRangeTest) cfg = heltecConfig.rangeTest;
    cfg.enabled = formBool("enabled");
    cfg.sender = formU32("sender", cfg.sender);
    cfg.save = formBool("save");
    cfg.clear_on_reboot = formBool("clear");
  } else if (section == "telemetry") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_telemetry_tag;
    meshtastic_ModuleConfig_TelemetryConfig& cfg = admin.set_module_config.payload_variant.telemetry;
    if (heltecConfig.hasTelemetry) cfg = heltecConfig.telemetry;
    cfg.device_telemetry_enabled = formBool("deviceEnabled");
    cfg.device_update_interval = formU32("deviceInterval", cfg.device_update_interval);
    cfg.environment_measurement_enabled = formBool("envEnabled");
    cfg.environment_screen_enabled = formBool("envScreen");
    cfg.environment_display_fahrenheit = formBool("envFahrenheit");
    cfg.environment_update_interval = formU32("envInterval", cfg.environment_update_interval);
    cfg.air_quality_enabled = formBool("airEnabled");
    cfg.air_quality_screen_enabled = formBool("airScreen");
    cfg.air_quality_interval = formU32("airInterval", cfg.air_quality_interval);
    cfg.power_measurement_enabled = formBool("powerEnabled");
    cfg.power_screen_enabled = formBool("powerScreen");
    cfg.power_update_interval = formU32("powerInterval", cfg.power_update_interval);
    cfg.health_measurement_enabled = formBool("healthEnabled");
    cfg.health_screen_enabled = formBool("healthScreen");
    cfg.health_update_interval = formU32("healthInterval", cfg.health_update_interval);
  } else if (section == "canned_message") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_canned_message_tag;
    meshtastic_ModuleConfig_CannedMessageConfig& cfg = admin.set_module_config.payload_variant.canned_message;
    if (heltecConfig.hasCannedMessage) cfg = heltecConfig.cannedMessage;
    cfg.enabled = formBool("enabled");
    cfg.rotary1_enabled = formBool("rotary");
    cfg.updown1_enabled = formBool("updown");
    cfg.inputbroker_pin_a = formU32("pinA", cfg.inputbroker_pin_a);
    cfg.inputbroker_pin_b = formU32("pinB", cfg.inputbroker_pin_b);
    cfg.inputbroker_pin_press = formU32("pinPress", cfg.inputbroker_pin_press);
    cfg.inputbroker_event_cw = (meshtastic_ModuleConfig_CannedMessageConfig_InputEventChar)formU32("cw", cfg.inputbroker_event_cw);
    cfg.inputbroker_event_ccw = (meshtastic_ModuleConfig_CannedMessageConfig_InputEventChar)formU32("ccw", cfg.inputbroker_event_ccw);
    cfg.inputbroker_event_press = (meshtastic_ModuleConfig_CannedMessageConfig_InputEventChar)formU32("press", cfg.inputbroker_event_press);
    formStr("source", cfg.allow_input_source, sizeof(cfg.allow_input_source), true);
    cfg.send_bell = formBool("sendBell");
  } else if (section == "audio") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_audio_tag;
    meshtastic_ModuleConfig_AudioConfig& cfg = admin.set_module_config.payload_variant.audio;
    if (heltecConfig.hasAudio) cfg = heltecConfig.audio;
    cfg.codec2_enabled = formBool("enabled");
    cfg.bitrate = (meshtastic_ModuleConfig_AudioConfig_Audio_Baud)formU32("bitrate", cfg.bitrate);
    cfg.ptt_pin = formU8("ptt", cfg.ptt_pin);
    cfg.i2s_ws = formU8("ws", cfg.i2s_ws);
    cfg.i2s_sd = formU8("sd", cfg.i2s_sd);
    cfg.i2s_din = formU8("din", cfg.i2s_din);
    cfg.i2s_sck = formU8("sck", cfg.i2s_sck);
  } else if (section == "neighbor_info") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_neighbor_info_tag;
    meshtastic_ModuleConfig_NeighborInfoConfig& cfg = admin.set_module_config.payload_variant.neighbor_info;
    if (heltecConfig.hasNeighborInfo) cfg = heltecConfig.neighborInfo;
    cfg.enabled = formBool("enabled");
    cfg.update_interval = formU32("interval", cfg.update_interval);
    cfg.transmit_over_lora = formBool("lora");
  } else if (section == "ambient_lighting") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_ambient_lighting_tag;
    meshtastic_ModuleConfig_AmbientLightingConfig& cfg = admin.set_module_config.payload_variant.ambient_lighting;
    if (heltecConfig.hasAmbientLighting) cfg = heltecConfig.ambientLighting;
    cfg.led_state = formBool("enabled");
    cfg.current = formU8("current", cfg.current);
    cfg.red = formU8("red", cfg.red);
    cfg.green = formU8("green", cfg.green);
    cfg.blue = formU8("blue", cfg.blue);
  } else if (section == "detection_sensor") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_detection_sensor_tag;
    meshtastic_ModuleConfig_DetectionSensorConfig& cfg = admin.set_module_config.payload_variant.detection_sensor;
    if (heltecConfig.hasDetectionSensor) cfg = heltecConfig.detectionSensor;
    cfg.enabled = formBool("enabled");
    formStr("name", cfg.name, sizeof(cfg.name), true);
    cfg.monitor_pin = formU8("pin", cfg.monitor_pin);
    cfg.minimum_broadcast_secs = formU32("minSecs", cfg.minimum_broadcast_secs);
    cfg.state_broadcast_secs = formU32("stateSecs", cfg.state_broadcast_secs);
    cfg.detection_trigger_type = (meshtastic_ModuleConfig_DetectionSensorConfig_TriggerType)formU32("trigger", cfg.detection_trigger_type);
    cfg.send_bell = formBool("sendBell");
    cfg.use_pullup = formBool("pullup");
  } else if (section == "paxcounter") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_paxcounter_tag;
    meshtastic_ModuleConfig_PaxcounterConfig& cfg = admin.set_module_config.payload_variant.paxcounter;
    if (heltecConfig.hasPaxcounter) cfg = heltecConfig.paxcounter;
    cfg.enabled = formBool("enabled");
    cfg.paxcounter_update_interval = formU32("interval", cfg.paxcounter_update_interval);
    cfg.wifi_threshold = formI32("wifi", cfg.wifi_threshold);
    cfg.ble_threshold = formI32("ble", cfg.ble_threshold);
  } else if (section == "status_message") {
    admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_statusmessage_tag;
    meshtastic_ModuleConfig_StatusMessageConfig& cfg = admin.set_module_config.payload_variant.statusmessage;
    if (heltecConfig.hasStatusMessage) cfg = heltecConfig.statusMessage;
    formStr("text", cfg.node_status, sizeof(cfg.node_status), true);
  } else {
    appendLine(eventLog, LOG_SIZE, "[local] unknown Heltec module section\n");
    return false;
  }

  bool ok = sendLocalAdmin(admin);
  char line[120];
  snprintf(line, sizeof(line), "[local] Heltec module %s %s\n", section.c_str(), ok ? "sent" : "failed");
  appendLine(eventLog, LOG_SIZE, line);
  return ok;
}

static bool sendHeltecTimezone(const String& tz) {
  if (!tz.length()) return false;
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
  admin.set_config.which_payload_variant = meshtastic_Config_device_tag;
  strlcpy(admin.set_config.payload_variant.device.tzdef, tz.c_str(), sizeof(admin.set_config.payload_variant.device.tzdef));
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec timezone sent\n" : "[local] Heltec timezone failed\n");
  return ok;
}

static bool sendHeltecCommit() {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_commit_edit_settings_tag;
  admin.commit_edit_settings = true;
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec commit sent\n" : "[local] Heltec commit failed\n");
  return ok;
}

static int8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool isValidPskByteSize(size_t len) {
  return len == 1 || len == 16 || len == 32;
}

static bool copyStoredChannelPsk(uint8_t index, meshtastic_ChannelSettings_psk_t& dest) {
  ChannelRecord* record = channelRecordByIndex(index);
  if (!record || !isValidPskByteSize(record->pskSize)) return false;
  dest.size = record->pskSize;
  memcpy(dest.bytes, record->psk, dest.size);
  return true;
}

static bool parseChannelPsk(const String& value, meshtastic_ChannelSettings_psk_t& dest) {
  String key = value;
  key.trim();
  if (!key.length()) return false;
  if (key.equalsIgnoreCase("none")) {
    dest.size = 1;
    dest.bytes[0] = 0;
    return true;
  }
  if (key.equalsIgnoreCase("default")) {
    dest.size = 1;
    dest.bytes[0] = 1;
    return true;
  }
  if (key.length() > 6 && key.substring(0, 6).equalsIgnoreCase("simple")) {
    int simple = key.substring(6).toInt();
    if (simple >= 1 && simple <= 9) {
      dest.size = 1;
      dest.bytes[0] = (uint8_t)(simple + 1);
      return true;
    }
  }
  if (key.length() > 2 && key[0] == '0' && (key[1] == 'x' || key[1] == 'X')) {
    String hex = key.substring(2);
    if ((hex.length() % 2) != 0) return false;
    size_t outLen = hex.length() / 2;
    if (!isValidPskByteSize(outLen) || outLen > sizeof(dest.bytes)) return false;
    for (size_t i = 0; i < outLen; i++) {
      int8_t hi = hexNibble(hex[i * 2]);
      int8_t lo = hexNibble(hex[i * 2 + 1]);
      if (hi < 0 || lo < 0) return false;
      dest.bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    dest.size = outLen;
    return true;
  }
  if (key.length() > 7 && key.substring(0, 7).equalsIgnoreCase("base64:")) {
    String encoded = key.substring(7);
    size_t decodedLen = 0;
    int rc = mbedtls_base64_decode(dest.bytes, sizeof(dest.bytes), &decodedLen,
                                   (const unsigned char*)encoded.c_str(), encoded.length());
    if (rc != 0 || !isValidPskByteSize(decodedLen)) return false;
    dest.size = decodedLen;
    return true;
  }
  if (key.length() == 16 || key.length() == 32) {
    dest.size = key.length();
    memcpy(dest.bytes, key.c_str(), dest.size);
    return true;
  }
  return false;
}

static bool sendHeltecChannelConfig(uint8_t index, const String& role, const String& name, const String& psk, bool uplink, bool downlink, int32_t positionPrecision) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_channel_tag;
  admin.set_channel.index = min<uint8_t>(index, 7);
  admin.set_channel.role = parseChannelRole(role);
  admin.set_channel.has_settings = admin.set_channel.role != meshtastic_Channel_Role_DISABLED;
  if (admin.set_channel.has_settings) {
    strlcpy(admin.set_channel.settings.name, name.c_str(), sizeof(admin.set_channel.settings.name));
    admin.set_channel.settings.uplink_enabled = uplink;
    admin.set_channel.settings.downlink_enabled = downlink;
    if (psk.length()) {
      if (!parseChannelPsk(psk, admin.set_channel.settings.psk)) {
        appendLine(eventLog, LOG_SIZE, "[local] channel PSK invalid; use 0x hex, base64, none, default, or 16/32 chars\n");
        return false;
      }
    } else if (!copyStoredChannelPsk(admin.set_channel.index, admin.set_channel.settings.psk)) {
      appendLine(eventLog, LOG_SIZE, "[local] channel PSK not loaded; refresh channel config or enter a PSK\n");
      return false;
    }
    if (positionPrecision >= 0) {
      admin.set_channel.settings.has_module_settings = true;
      admin.set_channel.settings.module_settings.position_precision = min<uint32_t>((uint32_t)positionPrecision, 32);
    }
  }
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec channel config sent\n" : "[local] Heltec channel config failed\n");
  return ok;
}

static bool sendChannelProvisionStep(uint8_t step) {
  if (step == 0) return sendHeltecLoraConfig("US", "MEDIUM_FAST", 3, 30);
  if (step == 1) {
    return sendHeltecChannelConfig(PUBLIC_CHANNEL_INDEX,
                                   "PRIMARY",
                                   CHANNEL_PROVISION_PUBLIC_NAME,
                                   "default",
                                   false,
                                   false,
                                   -1);
  }
  if (step == 2) {
    return sendHeltecChannelConfig(1,
                                   "SECONDARY",
                                   CHANNEL_PROVISION_FAMILY_NAME,
                                   CHANNEL_PROVISION_FAMILY_PSK,
                                   false,
                                   false,
                                   -1);
  }
  if (step >= 3 && step <= 8) {
    return sendHeltecChannelConfig((uint8_t)(step - 1),
                                   "DISABLED",
                                   "",
                                   "",
                                   false,
                                   false,
                                   -1);
  }
  if (step == 9) return sendHeltecCommit();
  if (step == 10) return sendConfigRequest();
  return false;
}

static void serviceOneTimeChannelProvision() {
#if ONE_TIME_CHANNEL_PROVISION
  if (channelProvisionDone) return;
  uint32_t now = millis();
  if ((int32_t)(now - nextChannelProvisionMs) < 0) return;
  if (channelProvisionStep == 0 && stats.myNodeNum == 0 && now < 30000) {
    nextChannelProvisionMs = now + 1000;
    return;
  }

  if (channelProvisionStep == 0) {
    appendLine(eventLog, LOG_SIZE, "[provision] applying Public and Family channel set\n");
  }

  bool ok = sendChannelProvisionStep(channelProvisionStep);
  char line[112];
  snprintf(line, sizeof(line), "[provision] step %u %s\n",
           (unsigned)channelProvisionStep,
           ok ? "sent" : "failed");
  appendLine(eventLog, LOG_SIZE, line);

  if (!ok) {
    nextChannelProvisionMs = now + CHANNEL_PROVISION_RETRY_MS;
    return;
  }

  if (channelProvisionStep >= CHANNEL_PROVISION_FINAL_STEP) {
    prefs.putBool(CHANNEL_PROVISION_PREF_KEY, true);
    channelProvisionDone = true;
    appendLine(eventLog, LOG_SIZE, "[provision] complete; Public/Family saved\n");
    return;
  }

  channelProvisionStep++;
  nextChannelProvisionMs = now + CHANNEL_PROVISION_STEP_MS;
#endif
}

static void updateTelemetry(uint32_t from, const meshtastic_Data& data) {
  meshtastic_Telemetry telemetry = meshtastic_Telemetry_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
  if (!pb_decode(&stream, meshtastic_Telemetry_fields, &telemetry)) return;

  char line[160];
  if (telemetry.which_variant == meshtastic_Telemetry_device_metrics_tag) {
    NodeRecord* node = findOrCreateNode(from);
    if (node) {
      node->hasDeviceMetrics = true;
      node->batteryLevel = telemetry.variant.device_metrics.battery_level;
      node->voltage = telemetry.variant.device_metrics.voltage;
      node->channelUtilization = telemetry.variant.device_metrics.channel_utilization;
      node->airUtilTx = telemetry.variant.device_metrics.air_util_tx;
      node->uptimeSeconds = telemetry.variant.device_metrics.uptime_seconds;
      node->lastTelemetryMs = millis();
    }
    lastTelemetryMs = millis();
    stats.batteryLevel = telemetry.variant.device_metrics.battery_level;
    stats.voltage = telemetry.variant.device_metrics.voltage;
    stats.channelUtilization = telemetry.variant.device_metrics.channel_utilization;
    stats.airUtilTx = telemetry.variant.device_metrics.air_util_tx;
    stats.uptimeSeconds = telemetry.variant.device_metrics.uptime_seconds;
    snprintf(line, sizeof(line), "[%s] battery %lu%% %.2fV\n",
             nodeName(from), (unsigned long)stats.batteryLevel, stats.voltage);
    appendLine(eventLog, LOG_SIZE, line);
    appendPacketEvent(line);
  } else if (telemetry.which_variant == meshtastic_Telemetry_local_stats_tag) {
    lastTelemetryMs = millis();
    stats.packetsRx = telemetry.variant.local_stats.num_packets_rx;
    stats.packetsTx = telemetry.variant.local_stats.num_packets_tx;
    stats.onlineNodes = telemetry.variant.local_stats.num_online_nodes;
    stats.totalNodes = telemetry.variant.local_stats.num_total_nodes;
    appendLine(eventLog, LOG_SIZE, "[radio] local stats updated\n");
    appendPacketEvent("[telemetry] local stats updated\n");
  }
  markUiDirty(UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD | UI_DIRTY_SYSTEM);
}

static void handleDecodedPacket(const meshtastic_MeshPacket& packet) {
  NodeRecord* node = findOrCreateNode(packet.from);
  if (node) {
    node->lastHeardMs = millis();
    node->snr = packet.rx_snr;
    node->rssi = packet.rx_rssi;
    node->lastChannel = packet.channel;
    node->packetsHeard++;
    node->hopsAway = packet.hop_start > packet.hop_limit ? packet.hop_start - packet.hop_limit : 0;
    markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD | UI_DIRTY_SYSTEM);
  }

  if (packet.which_payload_variant == meshtastic_MeshPacket_encrypted_tag) {
    encryptedPackets++;
    if (node) {
      node->encryptedPackets++;
      node->lastPortNum = 0;
    }
    char line[96];
    snprintf(line, sizeof(line), "[%s] encrypted packet\n", nodeName(packet.from));
    appendLine(eventLog, LOG_SIZE, line);
    appendPacketEvent(line);
    return;
  }

  if (packet.which_payload_variant != meshtastic_MeshPacket_decoded_tag) return;

  const meshtastic_Data& data = packet.decoded;
  char line[320];

  if (data.portnum == meshtastic_PortNum_ROUTING_APP) {
    meshtastic_Routing routing = meshtastic_Routing_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
    TxRecord* record = findTxRecord(data.request_id);
    if (record && packet.to == stats.myNodeNum &&
        pb_decode(&stream, meshtastic_Routing_fields, &routing) &&
        routing.which_variant == meshtastic_Routing_error_reason_tag) {
      bool success = routing.error_reason == meshtastic_Routing_Error_NONE;
      // Local firmware synthesizes broadcast implicit ACKs. For direct messages,
      // accept success only from the intended destination, including self-tests.
      bool sourceAllowed = FieldStatus::acceptRouting(record->direct, record->to, stats.myNodeNum, packet.from, packet.to, routing.error_reason);
      if (sourceAllowed && (success || record->delivery != FieldStatus::Delivery::Acknowledged)) {
        record->delivery = success ? FieldStatus::Delivery::Acknowledged : FieldStatus::Delivery::Failed;
        record->errorCode = (int16_t)routing.error_reason;
        record->queueError = false;
        markUiDirty(UI_DIRTY_TX | UI_DIRTY_DASHBOARD);
      }
    }
    lastPortNum = data.portnum;
  } else if (data.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP ||
      data.portnum == meshtastic_PortNum_TEXT_MESSAGE_COMPRESSED_APP) {
    textPackets++;
    lastPortNum = data.portnum;
    if (node) {
      node->textPackets++;
      node->lastPortNum = data.portnum;
    }
    bool fromThisNode = stats.myNodeNum != 0 && packet.from == stats.myNodeNum;
    if (!fromThisNode && isDirectAddress(packet.to) && taDirectTo) {
      char fromText[12];
      snprintf(fromText, sizeof(fromText), "!%08lX", (unsigned long)packet.from);
      lv_textarea_set_text(taDirectTo, fromText);
    }
    bool compressedText = data.portnum == meshtastic_PortNum_TEXT_MESSAGE_COMPRESSED_APP;
    const uint8_t* chatBytes = data.payload.bytes;
    size_t chatLen = data.payload.size;
    static const char compressedNotice[] = "(compressed text received)";
    if (compressedText) {
      chatBytes = (const uint8_t*)compressedNotice;
      chatLen = strlen(compressedNotice);
    }
    bool localEcho = !compressedText && fromThisNode && isRecentLocalEcho(packet.channel, packet.to, data.payload.bytes, data.payload.size);
    if (localEcho) {
      markTxEchoSeen(packet.channel, packet.to, data.payload.bytes, data.payload.size);
    } else {
      appendChatMessage(packet.channel, packet.to, fromThisNode ? "me" : nodeName(packet.from), chatBytes, chatLen);
      if (!fromThisNode) queueTone(isDirectAddress(packet.to) ? TONE_DIRECT : TONE_MESSAGE);
    }
    snprintf(line, sizeof(line), "[%s ch%u %s%s] %.*s\n",
             fromThisNode ? "me" : nodeName(packet.from),
             packet.channel,
             isPrivateChannel(packet.channel) ? "secondary" : "primary",
             compressedText ? " compressed" : "",
             (int)chatLen,
             chatBytes);
    appendLine(eventLog, LOG_SIZE, line);
    appendPacketEvent(line);
  } else if (data.portnum == meshtastic_PortNum_TELEMETRY_APP) {
    telemetryPackets++;
    lastPortNum = data.portnum;
    if (node) {
      node->telemetryPackets++;
      node->lastPortNum = data.portnum;
    }
    updateTelemetry(packet.from, data);
  } else if (data.portnum == meshtastic_PortNum_POSITION_APP) {
    positionPackets++;
    lastPortNum = data.portnum;
    if (node) {
      node->positionPackets++;
      node->lastPortNum = data.portnum;
    }
    meshtastic_Position position = meshtastic_Position_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
    if (pb_decode(&stream, meshtastic_Position_fields, &position) &&
        updateNodePosition(packet.from, position, "position packet")) {
      remotePositionPackets++;
      snprintf(line, sizeof(line), "[%s] position %.5f %.5f\n",
               nodeName(packet.from),
               position.latitude_i / 10000000.0,
               position.longitude_i / 10000000.0);
      appendPacketEvent(line);
    } else {
      appendLine(eventLog, LOG_SIZE, "[radio] position packet without usable lat/lon\n");
      appendPacketEvent("[position] unusable lat/lon\n");
    }
  } else if (data.portnum == meshtastic_PortNum_NODEINFO_APP) {
    nodeInfoPackets++;
    lastPortNum = data.portnum;
    if (node) node->lastPortNum = data.portnum;
    meshtastic_User user = meshtastic_User_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
    if (pb_decode(&stream, meshtastic_User_fields, &user)) {
      NodeRecord* known = findOrCreateNode(packet.from);
      if (known) {
        strncpy(known->name, user.long_name, sizeof(known->name) - 1);
        known->name[sizeof(known->name) - 1] = '\0';
        markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD);
      }
      snprintf(line, sizeof(line), "[radio] node %08lX is %.39s\n",
               (unsigned long)packet.from, user.long_name);
      appendLine(eventLog, LOG_SIZE, line);
      appendPacketEvent(line);
    }
  } else {
    lastPortNum = data.portnum;
    otherFrames++;
    if (node) node->lastPortNum = data.portnum;
    snprintf(line, sizeof(line), "[%s] port %d payload %u bytes\n",
             nodeName(packet.from), data.portnum, data.payload.size);
    appendLine(eventLog, LOG_SIZE, line);
    appendPacketEvent(line);
  }
}

static void decodeFromRadio(const uint8_t* payload, size_t len) {
  meshtastic_FromRadio fromRadio = meshtastic_FromRadio_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(payload, len);
  if (!pb_decode(&stream, meshtastic_FromRadio_fields, &fromRadio)) {
    decodeErrors++;
    appendLine(eventLog, LOG_SIZE, "[radio] protobuf decode failed\n");
    return;
  }

  framesDecoded++;
  if (fromRadio.which_payload_variant != 0) {
    radioResponseSeen = true;
    lastRadioResponseMs = millis();
  }
  if (fromRadio.which_payload_variant == meshtastic_FromRadio_packet_tag) {
    handleDecodedPacket(fromRadio.packet);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_my_info_tag) {
    if (stats.myNodeNum && stats.myNodeNum != fromRadio.my_info.my_node_num) resetGpsRelayLink();
    stats.myNodeNum = fromRadio.my_info.my_node_num;
    char line[96];
    snprintf(line, sizeof(line), "[radio] connected as !%08lX\n", (unsigned long)stats.myNodeNum);
    appendLine(eventLog, LOG_SIZE, line);
    if (forcedOwnerNamePending && stats.myNodeNum) {
      forcedOwnerNamePending = false;
      sendHeltecOwnerName(interfaceDeviceName, String(interfaceDeviceName).substring(0, min<size_t>(4, strlen(interfaceDeviceName))));
    }
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_node_info_tag) {
    nodeInfoPackets++;
    NodeRecord* node = findOrCreateNode(fromRadio.node_info.num);
    if (node) {
      strncpy(node->name, fromRadio.node_info.user.long_name, sizeof(node->name) - 1);
      node->name[sizeof(node->name) - 1] = '\0';
      node->isFavorite = fromRadio.node_info.is_favorite;
      node->snr = fromRadio.node_info.snr;
      node->lastHeardMs = millis();
      node->packetsHeard++;
      node->lastPortNum = meshtastic_PortNum_NODEINFO_APP;
      markUiDirty(UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD);
    }
    if (fromRadio.node_info.has_position) {
      positionPackets++;
      if (node) node->positionPackets++;
      if (updateNodePosition(fromRadio.node_info.num, fromRadio.node_info.position, "node info")) {
        remotePositionPackets++;
      }
    }
    if (fromRadio.node_info.has_device_metrics) {
      if (node) {
        node->hasDeviceMetrics = true;
        node->batteryLevel = fromRadio.node_info.device_metrics.battery_level;
        node->voltage = fromRadio.node_info.device_metrics.voltage;
        node->channelUtilization = fromRadio.node_info.device_metrics.channel_utilization;
        node->airUtilTx = fromRadio.node_info.device_metrics.air_util_tx;
        node->uptimeSeconds = fromRadio.node_info.device_metrics.uptime_seconds;
        node->lastTelemetryMs = millis();
        node->telemetryPackets++;
      }
      lastTelemetryMs = millis();
      stats.batteryLevel = fromRadio.node_info.device_metrics.battery_level;
      stats.voltage = fromRadio.node_info.device_metrics.voltage;
      stats.channelUtilization = fromRadio.node_info.device_metrics.channel_utilization;
      stats.airUtilTx = fromRadio.node_info.device_metrics.air_util_tx;
      stats.uptimeSeconds = fromRadio.node_info.device_metrics.uptime_seconds;
    }
    char line[96];
    snprintf(line, sizeof(line), "[nodeinfo] %s %.1f dB\n",
             nodeName(fromRadio.node_info.num),
             node ? node->snr : 0.0f);
    appendPacketEvent(line);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_queueStatus_tag) {
    const auto& queue = fromRadio.queueStatus;
    if (gpsRelay.queueResult(queue.mesh_packet_id, queue.res)) forcePageRefresh = true;
    TxRecord* record = findTxRecord(queue.mesh_packet_id);
    if (record && record->delivery != FieldStatus::Delivery::Acknowledged && record->delivery != FieldStatus::Delivery::Failed) {
      record->delivery = FieldStatus::queueResult(record->delivery, queue.res);
      if (record->delivery == FieldStatus::Delivery::Failed) {
        record->queueError = true;
        record->errorCode = queue.res;
      }
      markUiDirty(UI_DIRTY_TX | UI_DIRTY_DASHBOARD);
    }
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_log_record_tag) {
    char line[420];
    snprintf(line, sizeof(line), "[node log] %.380s\n", fromRadio.log_record.message);
    appendLine(eventLog, LOG_SIZE, line);
    appendPacketEvent(line);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_channel_tag) {
    configFrames++;
    updateChannelRecord(fromRadio.channel);
    appendPacketEvent("[config] channel record\n");
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_config_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_moduleConfig_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_config_complete_id_tag) {
    configFrames++;
    if (fromRadio.which_payload_variant == meshtastic_FromRadio_config_tag) {
      cacheHeltecConfig(fromRadio.config);
    } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_moduleConfig_tag) {
      cacheHeltecModuleConfig(fromRadio.moduleConfig);
    } else {
      appendPacketEvent("[config] received config complete\n");
      onGpsRelayConfigComplete(fromRadio.config_complete_id);
    }
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_rebooted_tag && fromRadio.rebooted) {
    resetGpsRelayLink();
    stats.myNodeNum = 0;
    sendConfigRequest();
  } else {
    otherFrames++;
    appendPacketEvent("[radio] other FromRadio frame\n");
  }
}

static void pollLoRa() {
  static Reliability::FrameParser<FRAME_MAX> parser;
  if (parser.expire(millis())) serialFrameTimeouts++;

  while (SerialLoRa.available()) {
    uint8_t c = SerialLoRa.read();
    bytesFromRadio++;
    lastByteMs = millis();
    if (c == 0x94) magic1Count++;

    if ((c >= 32 && c <= 126) || c == '\n' || c == '\r') {
      size_t len = strlen(serialPeek);
      if (len >= sizeof(serialPeek) - 2) {
        memmove(serialPeek, serialPeek + 32, sizeof(serialPeek) - 33);
        serialPeek[sizeof(serialPeek) - 33] = '\0';
        len = strlen(serialPeek);
      }
      serialPeek[len++] = (c == '\r') ? ' ' : (char)c;
      serialPeek[len] = '\0';
    }

    auto result = parser.feed(c, lastByteMs);
    if (result == Reliability::FrameParser<FRAME_MAX>::HEADER) {
      magic2Count++;
    } else if (result == Reliability::FrameParser<FRAME_MAX>::INVALID_LENGTH) {
      invalidFrameLengths++;
    } else if (result == Reliability::FrameParser<FRAME_MAX>::LENGTH_ACCEPTED) {
      streamFrames++;
    } else if (result == Reliability::FrameParser<FRAME_MAX>::COMPLETE) {
      decodeFromRadio(parser.payload(), parser.length());
    }
  }
}

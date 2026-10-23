#pragma once
#include <atomic>
#include "../NotificationSound.h"

// Shared runtime state; included once by the main sketch.
LV_FONT_DECLARE(comic_neue_12);
LV_FONT_DECLARE(comic_neue_14);

HardwareSerial SerialLoRa(2);
HardwareSerial SerialGPS(1);
WebServer server(80);
Preferences prefs;
TFT_eSPI tft;
TinyGPSPlus localGps;
TinyGPSCustom gpsGpInView(localGps, "GPGSV", 3);
TinyGPSCustom gpsGnInView(localGps, "GNGSV", 3);

struct NodeRecord {
  uint32_t num = 0;
  char name[40] = "";
  float snr = 0.0f;
  int32_t rssi = 0;
  uint8_t hopsAway = 0;
  uint8_t lastChannel = 0;
  uint32_t lastPortNum = 0;
  uint32_t lastHeardMs = 0;
  uint32_t packetsHeard = 0;
  uint32_t textPackets = 0;
  uint32_t telemetryPackets = 0;
  uint32_t positionPackets = 0;
  uint32_t encryptedPackets = 0;
  bool hasDeviceMetrics = false;
  uint32_t batteryLevel = 0;
  float voltage = 0.0f;
  float channelUtilization = 0.0f;
  float airUtilTx = 0.0f;
  uint32_t uptimeSeconds = 0;
  uint32_t lastTelemetryMs = 0;
  bool hasPosition = false;
  double latitude = 0.0;
  double longitude = 0.0;
  int32_t altitude = 0;
  uint32_t lastPositionMs = 0;
  bool isFavorite = false;
};

struct ChannelRecord {
  int8_t index = -1;
  char name[12] = "";
  char role[16] = "";
  bool enabled = false;
  bool uplink = false;
  bool downlink = false;
  uint8_t pskSize = 0;
  uint8_t psk[32] = {0};
  bool hasPositionPrecision = false;
  uint32_t positionPrecision = 0;
};

struct TxRecord {
  uint32_t packetId = 0;
  uint32_t retriedAs = 0;
  FieldStatus::Delivery delivery = FieldStatus::Delivery::Queued;
  int16_t errorCode = 0;
  bool queueError = false;
  char text[234] = "";
  uint32_t to = 0xFFFFFFFFUL;
  uint8_t channel = 0;
  uint32_t sentMs = 0;
  bool direct = false;
  bool ok = false;
  bool echoSeen = false;
};

struct HeltecConfigCache {
  bool hasLora = false;
  bool hasDevice = false;
  bool hasPosition = false;
  bool hasPower = false;
  bool hasNetwork = false;
  bool hasDisplay = false;
  bool hasBluetooth = false;
  bool hasSecurity = false;
  bool hasSerial = false;
  bool hasMqtt = false;
  bool hasExternalNotification = false;
  bool hasStoreForward = false;
  bool hasRangeTest = false;
  bool hasTelemetry = false;
  bool hasCannedMessage = false;
  bool hasAudio = false;
  bool hasNeighborInfo = false;
  bool hasAmbientLighting = false;
  bool hasDetectionSensor = false;
  bool hasPaxcounter = false;
  bool hasStatusMessage = false;
  uint32_t lastConfigMs = 0;
  uint32_t lastModuleMs = 0;
  meshtastic_Config_LoRaConfig lora = meshtastic_Config_LoRaConfig_init_zero;
  meshtastic_Config_DeviceConfig device = meshtastic_Config_DeviceConfig_init_zero;
  meshtastic_Config_PositionConfig position = meshtastic_Config_PositionConfig_init_zero;
  meshtastic_Config_PowerConfig power = meshtastic_Config_PowerConfig_init_zero;
  meshtastic_Config_NetworkConfig network = meshtastic_Config_NetworkConfig_init_zero;
  meshtastic_Config_DisplayConfig display = meshtastic_Config_DisplayConfig_init_zero;
  meshtastic_Config_BluetoothConfig bluetooth = meshtastic_Config_BluetoothConfig_init_zero;
  meshtastic_Config_SecurityConfig security = meshtastic_Config_SecurityConfig_init_zero;
  meshtastic_ModuleConfig_SerialConfig serial = meshtastic_ModuleConfig_SerialConfig_init_zero;
  meshtastic_ModuleConfig_MQTTConfig mqtt = meshtastic_ModuleConfig_MQTTConfig_init_zero;
  meshtastic_ModuleConfig_ExternalNotificationConfig externalNotification = meshtastic_ModuleConfig_ExternalNotificationConfig_init_zero;
  meshtastic_ModuleConfig_StoreForwardConfig storeForward = meshtastic_ModuleConfig_StoreForwardConfig_init_zero;
  meshtastic_ModuleConfig_RangeTestConfig rangeTest = meshtastic_ModuleConfig_RangeTestConfig_init_zero;
  meshtastic_ModuleConfig_TelemetryConfig telemetry = meshtastic_ModuleConfig_TelemetryConfig_init_zero;
  meshtastic_ModuleConfig_CannedMessageConfig cannedMessage = meshtastic_ModuleConfig_CannedMessageConfig_init_zero;
  meshtastic_ModuleConfig_AudioConfig audio = meshtastic_ModuleConfig_AudioConfig_init_zero;
  meshtastic_ModuleConfig_NeighborInfoConfig neighborInfo = meshtastic_ModuleConfig_NeighborInfoConfig_init_zero;
  meshtastic_ModuleConfig_AmbientLightingConfig ambientLighting = meshtastic_ModuleConfig_AmbientLightingConfig_init_zero;
  meshtastic_ModuleConfig_DetectionSensorConfig detectionSensor = meshtastic_ModuleConfig_DetectionSensorConfig_init_zero;
  meshtastic_ModuleConfig_PaxcounterConfig paxcounter = meshtastic_ModuleConfig_PaxcounterConfig_init_zero;
  meshtastic_ModuleConfig_StatusMessageConfig statusMessage = meshtastic_ModuleConfig_StatusMessageConfig_init_zero;
};

struct DeviceStats {
  uint32_t myNodeNum = 0;
  uint32_t uptimeSeconds = 0;
  uint32_t batteryLevel = 0;
  float voltage = 0.0f;
  float channelUtilization = 0.0f;
  float airUtilTx = 0.0f;
  uint32_t packetsRx = 0;
  uint32_t packetsTx = 0;
  uint16_t onlineNodes = 0;
  uint16_t totalNodes = 0;
};

enum NodeSortMode : uint8_t {
  NODE_SORT_LAST_HEARD,
  NODE_SORT_SNR,
  NODE_SORT_DISTANCE,
  NODE_SORT_NAME
};

enum NodeFilterMode : uint8_t {
  NODE_FILTER_ALL,
  NODE_FILTER_ACTIVE,
  NODE_FILTER_POSITIONED,
  NODE_FILTER_STALE,
  NODE_FILTER_NEARBY
};

enum NodeListViewMode : uint8_t {
  NODE_LIST_VIEW_COMPACT,
  NODE_LIST_VIEW_DETAILED
};

enum MapCenterMode : uint8_t {
  MAP_CENTER_CURRENT_DEVICE,
  MAP_CENTER_SELECTED_NODE,
  MAP_CENTER_LAST_LOCATION
};

enum ToneKind : uint8_t {
  TONE_NONE,
  TONE_MESSAGE,
  TONE_DIRECT,
  TONE_TEST
};

enum ToneCodecProfile : uint8_t {
  TONE_CODEC_PROFILE_DRIVER,
  TONE_CODEC_PROFILE_REF_HP,
  TONE_CODEC_PROFILE_REF_LINE,
  TONE_CODEC_PROFILE_LEGACY,
  TONE_CODEC_PROFILE_COUNT
};

struct GpsStats {
  bool valid = false;
  bool hasSats = false;
  bool hasFixQuality = false;
  bool hasFixType = false;
  bool hasTimestamp = false;
  bool hasPdop = false;
  bool hasHdop = false;
  bool hasVdop = false;
  bool hasAccuracy = false;
  bool hasGroundSpeed = false;
  bool hasGroundTrack = false;
  bool hasNextUpdate = false;
  bool hasPrecision = false;
  bool hasAltitudeHae = false;
  bool hasGeoidalSeparation = false;
  uint32_t from = 0;
  double latitude = 0.0;
  double longitude = 0.0;
  int32_t altitude = 0;
  int32_t altitudeHae = 0;
  int32_t geoidalSeparation = 0;
  uint32_t sats = 0;
  uint32_t fixQuality = 0;
  uint32_t fixType = 0;
  uint32_t timestamp = 0;
  uint32_t pdop = 0;
  uint32_t hdop = 0;
  uint32_t vdop = 0;
  uint32_t accuracyMm = 0;
  uint32_t groundSpeed = 0;
  uint32_t groundTrack = 0;
  uint32_t sensorId = 0;
  uint32_t nextUpdate = 0;
  uint32_t seqNumber = 0;
  uint32_t precisionBits = 0;
  uint32_t lastUpdateMs = 0;
  char sourceKind[24] = "none";
};

struct LocalBatteryStats {
  bool gaugePresent = false;
  bool gaugeSocReliable = true;
  uint32_t lastGaugeMs = 0;
  uint32_t lastSampleMs = 0;
  uint32_t rawMv = 0;
  uint32_t rawPackMv = 0;
  uint32_t filteredPackMv = 0;
  uint32_t batteryMv = 5000;
  uint32_t learnedBatteryMv = 5000;
  int32_t deltaMvPerMin = 0;
  int32_t deltaMvPerMinTenths = 0;
  bool chargeRateValid = false;
  float chargeRatePercentHr = 0.0f;
  int32_t instantDeltaMv = 0;
  int percent = 100;
  int gaugePercent = 100;
  uint8_t trendSampleCount = 0;
  char powerState[36] = "ext power";
  char percentSource[32] = "MAX17048";
  uint16_t gaugeVersion = 0;
  uint16_t rawGaugeSoc = 0;
  float gaugeSoc = 100.0f;
  bool charging = false;
  uint32_t previousPackMv = 0;
  uint32_t previousSampleMs = 0;
  uint32_t lastRuntimeSaveMs = 0;
  bool runtimeSaveAttempted = false;
  float savedDischargeRate = 0;
  uint8_t trendHead = 0;
  uint8_t trendCount = 0;
  uint32_t trendMv[12] = {};
  uint32_t trendMs[12] = {};
  uint32_t sampleCount = 0;
  uint32_t voltageRejectedSamples = 0;
  uint8_t voltageStableSamples = 0;
  bool voltageUnstable = false;
  uint32_t lowVoltageSinceMs = 0;
  uint32_t criticalVoltageSinceMs = 0;
  bool voltageLowSustained = false;
  bool voltageCriticalSustained = false;
  bool batteryLow = false;
  bool batteryCritical = false;
};

struct SdStorageStats {
  bool available = false;
  uint64_t cardSizeBytes = 0;
  uint64_t totalBytes = 0;
  uint64_t usedBytes = 0;
  uint32_t writes = 0;
  uint32_t writeErrors = 0;
  char status[48] = "not mounted";
  char cardType[12] = "none";
};

struct MapCacheHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t width;
  uint16_t height;
  int16_t zoom;
  int32_t tileX;
  int32_t tileY;
  int32_t pixelX;
  int32_t pixelY;
  double lat;
  double lon;
  uint8_t tileFound;
  uint8_t reserved[7];
};

static constexpr size_t LOG_SIZE = 8192;
static constexpr size_t CHAT_SIZE = 4096;
static constexpr size_t PACKET_LOG_SIZE = 4096;
static constexpr size_t STATUS_JSON_RESERVE = LOG_SIZE + (CHAT_SIZE * 3) + 12288;
static constexpr size_t MAX_NODES = 64;
static constexpr size_t TX_HISTORY_COUNT = 10;
static constexpr double NODE_NEARBY_METERS = 5000.0;
static constexpr size_t MAP_DOT_COUNT = MAX_NODES + 1;
static constexpr size_t MAX_CHANNELS = 8;
static constexpr size_t FRAME_MAX = 512;
static constexpr int STATUS_BAR_H = 20;
static constexpr int NAV_BAR_H = 40;
static constexpr int MAP_PLOT_W = SCREEN_W - 16;
static constexpr int MAP_PLOT_H_NORMAL = 136;
static constexpr int MAP_PLOT_H_EXPANDED = 214;
static constexpr int MAP_PLOT_H = MAP_PLOT_H_EXPANDED;
static constexpr size_t MAP_CANVAS_PIXELS = MAP_PLOT_W * MAP_PLOT_H;
static constexpr size_t MAP_CANVAS_BYTES = MAP_CANVAS_PIXELS * sizeof(lv_color_t);
static constexpr int MAP_TILE_SIZE = 256;
static constexpr int MAP_TILE_MIN_ZOOM = 10;
static constexpr int MAP_TILE_MAX_ZOOM = 14;
static constexpr uint32_t MAP_CACHE_MAGIC = 0x4D415031UL;
static constexpr uint16_t MAP_CACHE_VERSION = 1;

static constexpr size_t MAP_TILE_CACHE_SLOTS = 8;
static constexpr size_t MAP_TILE_BYTES = MAP_TILE_SIZE * MAP_TILE_SIZE * sizeof(uint16_t);
static uint16_t* mapTileCacheBuffer = nullptr;
static MapTiles::LruIndex<MAP_TILE_CACHE_SLOTS> mapTileIndex;
static char mapTileCacheRoot[64] = "";
static uint32_t mapTileCacheContext = 0; // Worker-owned.
static std::atomic<uint32_t> mapStorageGeneration{1};
static std::atomic<uint32_t> mapTileCacheHits{0}, mapTileCacheMisses{0}, mapTileCacheLoads{0};
static std::atomic<uint32_t> mapTileCacheEvictions{0}, mapTileCacheReadErrors{0}, mapTileCacheResident{0};
static std::atomic<uint32_t> mapTileSdReadCalls{0}, mapTileSdReadBytes{0};
static uint32_t lastMapTileReadUs = 0;
static bool mapBenchmarkRunning = false;

struct MapBenchmarkSample {
  uint32_t durationUs = 0, readCalls = 0, readBytes = 0;
  bool ok = false, pixelsMatch = false;
};

static MapBenchmarkSample lastMapBenchmark[7];
static bool lastMapBenchmarkAvailable = false, lastMapBenchmarkPassed = false;

struct MapRenderJob {
  uint32_t viewRevision = 0;
  uint32_t storageGeneration = 0;
  bool benchmark = false;
  uint32_t id = 0;
  double lat = 0.0;
  double lon = 0.0;
  int zoom = MAP_TILE_MAX_ZOOM;
  uint16_t plotH = MAP_PLOT_H_NORMAL;
  long centerTileX = 0;
  long centerTileY = 0;
  int centerPixelX = 0;
  int centerPixelY = 0;
  char tileRoot[64] = "";
};

struct MapRenderResult {
  MapCoverage::Profile coverage;
  uint8_t drawnMask = 0;
  uint32_t viewRevision = 0;
  uint32_t storageGeneration = 0;
  uint32_t tileReadUs = 0;
  bool benchmark = false;
  bool benchmarkPassed = false;
  MapBenchmarkSample samples[7] = {};
  uint32_t id = 0;
  double lat = 0.0;
  double lon = 0.0;
  int zoom = MAP_TILE_MAX_ZOOM;
  uint16_t plotH = MAP_PLOT_H_NORMAL;
  long centerTileX = 0;
  long centerTileY = 0;
  int centerPixelX = 0;
  int centerPixelY = 0;
  bool centerTileFound = false;
  bool drewTiles = false;
  uint32_t durationMs = 0;
  char status[48] = "";
};

static constexpr int8_t PUBLIC_CHANNEL_INDEX = 0;
static constexpr uint32_t BROADCAST_ADDR = 0xFFFFFFFFUL;
#ifndef ONE_TIME_CHANNEL_PROVISION
#define ONE_TIME_CHANNEL_PROVISION 0
#endif
static const char* CHANNEL_PROVISION_PREF_KEY = "chanProv2";
static const char* CHANNEL_PROVISION_PUBLIC_NAME = "MediumFast";
static const char* CHANNEL_PROVISION_FAMILY_NAME = "Family";
static const char* CHANNEL_PROVISION_FAMILY_PSK = "0x66e38d5a81f3136d16d6d5cc1f872298";
static constexpr uint32_t CHANNEL_PROVISION_BOOT_DELAY_MS = 8000;
static constexpr uint32_t CHANNEL_PROVISION_RETRY_MS = 3000;
static constexpr uint32_t CHANNEL_PROVISION_STEP_MS = 700;
static constexpr uint8_t CHANNEL_PROVISION_FINAL_STEP = 10;
static constexpr uint32_t RADIO_API_HEARTBEAT_MS = 30000;
static constexpr uint32_t RADIO_API_IDLE_RESYNC_MS = 360000;
static constexpr uint32_t RADIO_API_RESYNC_COOLDOWN_MS = 300000;
static const char* SETUP_DEVICE_NAME = "s3-lora-interface";
static const char* SETUP_AUTH_USER = "admin";
static const char* SETUP_AUTH_PASS = "setup1234";
static const char* SD_DIR = "/s3-lora";
static const char* SD_EVENTS_PATH = "/s3-lora/events.log";
static const char* SD_PUBLIC_CHAT_PATH = "/s3-lora/public_chat.log";
static const char* SD_SECONDARY_CHAT_PATH = "/s3-lora/secondary_chat.log";
static const char* SD_LEGACY_SECONDARY_CHAT_PATH = "/s3-lora/private_family_chat.log";
static const char* SD_DIRECT_CHAT_PATH = "/s3-lora/direct_messages.log";
static const char* SD_POSITIONS_PATH = "/s3-lora/positions.csv";
static const char* DEFAULT_MAP_TILE_ROOT = "/s3-lora/tiles";
static const char* SD_MAP_CACHE_PATH = "/s3-lora/map_cache.bin";
static const char* SD_LAST_LOCATION_PATH = "/s3-lora/last_location.txt";
static const char* SD_STATUS_SNAPSHOT_PATH = "/s3-lora/status_snapshot.json";
static constexpr size_t SD_APPEND_QUEUE_DEPTH = 12;
static constexpr size_t SD_APPEND_PATH_MAX = 64;
static constexpr size_t SD_APPEND_LINE_MAX = 512;
struct SdAppendItem {
  char path[SD_APPEND_PATH_MAX];
  char line[SD_APPEND_LINE_MAX];
};
static char interfaceDeviceName[33] = DEVICE_NAME;
static char interfaceHostname[33] = DEVICE_HOSTNAME;
static char interfaceApSsid[33] = INTERFACE_AP_SSID;
static char interfaceApPass[65] = INTERFACE_AP_PASS;
static char webUiUser[33] = WEBUI_AUTH_USER;
static char webUiPass[65] = WEBUI_AUTH_PASS;
static uint8_t interfaceApChannel = WIFI_AP_CHANNEL;
static bool otaRestartPending = false;
static uint32_t otaRestartAtMs = 0;
static bool otaUploadOk = false;
static constexpr uint32_t COLOR_BG = 0x050807;
static constexpr uint32_t COLOR_PANEL = 0x101816;
static constexpr uint32_t COLOR_INPUT = 0x07100D;
static constexpr uint32_t COLOR_BORDER = 0x24483E;
static constexpr uint32_t COLOR_TEXT = 0xF4FFF9;
static constexpr uint32_t COLOR_MUTED = 0x8AB7A6;
static constexpr uint32_t COLOR_ACCENT = 0x68FFC0;
static constexpr uint32_t COLOR_ACTION = 0x00C985;
static const lv_font_t* UI_FONT_SMALL = &comic_neue_12;
static const lv_font_t* UI_FONT_DEFAULT = &comic_neue_14;
#if LV_FONT_MONTSERRAT_16
static const lv_font_t* UI_FONT_READABLE = &lv_font_montserrat_16;
#else
static const lv_font_t* UI_FONT_READABLE = LV_FONT_DEFAULT;
#endif
static constexpr int UI_GAP = 6;
static constexpr int UI_PANEL_W = SCREEN_W - 12;
static constexpr int UI_ACTION_H = 36;
static constexpr int UI_ACTION_W = SCREEN_W - 18;
static constexpr int UI_INPUT_H = 38;
static constexpr int UI_TILE_W = 106;
static constexpr int UI_TILE_H = 42;

static char* eventLog = nullptr;
static char* packetLog = nullptr;
static char* publicChatLog = nullptr;
static char* familyChatLog = nullptr;
static char* directChatLog = nullptr;
static bool chatHistoryLoadedFromSd = false;
static size_t publicChatHistoryBytes = 0;
static size_t familyChatHistoryBytes = 0;
static size_t directChatHistoryBytes = 0;
static char lastLocalSentText[234];
static NodeRecord nodes[MAX_NODES];
static TxRecord txHistory[TX_HISTORY_COUNT];
static ChannelRecord channels[MAX_CHANNELS];
static HeltecConfigCache heltecConfig;
static size_t nodeCount = 0;
static size_t txHistoryCount = 0;
static size_t txHistoryNext = 0;
static int8_t privateChannelIndex = -1;
static DeviceStats stats;
static GpsStats gpsStats;
static LocalBatteryStats localBattery;
static SdStorageStats sdStorage;
static uint32_t framesDecoded = 0;
static uint32_t decodeErrors = 0;
static uint32_t bytesFromRadio = 0;
static uint32_t bytesToRadio = 0;
static uint32_t lastByteMs = 0;
static uint32_t lastRadioResponseMs = 0;
static bool radioResponseSeen = false;
static FieldStatus::RuntimeEstimate batteryRuntime;
static uint32_t magic1Count = 0;
static uint32_t magic2Count = 0;
static uint32_t streamFrames = 0;
static uint32_t invalidFrameLengths = 0;
static uint32_t serialFrameTimeouts = 0;
static bool channelProvisionDone = false;
static uint8_t channelProvisionStep = 0;
static uint32_t nextChannelProvisionMs = 0;
static bool nodeDetailScrollTopPending = false;
static uint32_t nodeDetailRenderedNode = 0;
static char nodeDetailLastText[960] = "";
static uint32_t textPackets = 0;
static uint32_t telemetryPackets = 0;
static uint32_t positionPackets = 0;
static uint32_t remotePositionPackets = 0;
static uint32_t nodeInfoPackets = 0;
static uint32_t encryptedPackets = 0;
static uint32_t configFrames = 0;
static uint32_t otherFrames = 0;
static uint32_t lastPortNum = 0;
static char serialPeek[96] = "";
static uint32_t lastTelemetryMs = 0;
static uint32_t lastConfigRequestMs = 0;
static uint8_t configRequestCount = 0;
static uint32_t nextConfigRequestId = 1;
static uint32_t nextMeshPacketId = 1;
static uint32_t heartbeatNonce = 0;
static uint32_t lastHeartbeatMs = 0;
static uint32_t heartbeatCount = 0;
static uint32_t lastApiResyncMs = 0;
static uint32_t apiResyncCount = 0;
static uint32_t gpsBytesFromLocal = 0;
static uint32_t lastLocalGpsByteMs = 0;
static uint32_t lastLocalGpsFixLogMs = 0;
static bool localGpsHasFix = false;
static char i2cProbeSummary[192] = "Tap Probe I2C Map to test the LCDWiki wiring.";
static char i2cProbeDetails[512] = "";
static char msgDiagStatus[192] = "Select two favorite nodes.";
static char msgDiagDetail[384] = "";
static uint32_t msgDiagTargetA = 0;
static uint32_t msgDiagTargetB = 0;
static uint32_t msgDiagLastTarget = 0;
static uint32_t msgDiagLastSentMs = 0;
static bool msgDiagLastSendOk = false;
static char msgDiagLastTargetName[48] = "";
static char msgDiagLastMessage[64] = "";
static uint32_t msgDiagFavoriteNodes[MAX_NODES];
static size_t msgDiagFavoriteCount = 0;
static uint32_t msgDiagTargetNodes[MAX_NODES];
static size_t msgDiagTargetCount = 0;
static bool msgDiagTargetUsesAllNodes = false;
static uint32_t msgDiagTargetSignature = 0;
static bool wifiEnabled = true;
static bool wifiApMode = true;
static bool forcedOwnerNamePending = FORCE_INTERFACE_SETTINGS != 0;
static bool wifiScanActive = false;
static bool wifiScanRequested = false;
static bool wifiScanStoppedWifi = false;
static uint32_t wifiScanRequestedMs = 0;
static uint32_t wifiScanStartedMs = 0;
static esp_err_t wifiScanStartResult = ESP_OK;
static volatile bool wifiScanTaskRunning = false;
static volatile bool wifiScanTaskDone = false;
static volatile int16_t wifiScanTaskStatus = WIFI_SCAN_FAILED;
static char wifiLocalSsid[33] = "";
static char wifiLocalPass[65] = "";
static constexpr size_t WIFI_SCAN_MAX_RESULTS = 16;
static char wifiScanSsids[WIFI_SCAN_MAX_RESULTS][33];
static int32_t wifiScanRssi[WIFI_SCAN_MAX_RESULTS];
static size_t wifiScanResultCount = 0;
static uint32_t wifiStartedMs = 0;
static uint32_t wifiStoppedMs = 0;
static uint32_t wifiToggleCount = 0;
static uint8_t backlightPercent = 10;
static bool backlightPwmReady = false;
static bool readableTextMode = false;
static constexpr uint32_t TONE_SAMPLE_RATE = 16000;
static constexpr uint16_t TONE_FRAMES_PER_SERVICE = 256;
static I2SClass toneI2s;
static bool toneI2sReady = false;
static bool toneCodecReady = false;
static bool toneSpeakerEnabled = false;
static bool toneActive = false;
static constexpr uint32_t TONE_SPEAKER_HOLD_MS = 650;
static ToneKind activeToneKind = TONE_NONE;
static ToneKind pendingToneKind = TONE_NONE;
static ToneStyle activeToneStyle = TONE_STYLE_BWOMP;
static ToneStyle toneMessageStyle = TONE_STYLE_BWOMP;
static ToneStyle toneDirectStyle = TONE_STYLE_DOUBLE;
static uint8_t toneBoostLevel = 0;
static uint8_t toneCodecProfile = TONE_CODEC_PROFILE_DRIVER;
static uint8_t toneVolumePercent = 50;
static uint32_t lastToneMs = 0;
static uint32_t toneCodecInitFailures = 0;
static uint32_t toneWriteCalls = 0;
static uint32_t toneBytesWritten = 0;
static uint32_t toneShortWrites = 0;
static uint32_t toneWriteFailures = 0;
static size_t toneLastWriteRequested = 0;
static size_t toneLastWriteBytes = 0;
static int toneLastI2sError = 0;
struct ToneRequest {
  ToneKind kind;
  ToneStyle style;
  uint32_t epoch;
  uint8_t volumeLimit = 100;
};
// The task owns PCM generation and I2S writes. Codec I2C and GPIO control stay
// on the main loop; recovery waits for the task's explicit pause acknowledgement.
struct ToneWorkerState {
  std::atomic<bool> pauseRequested{false}, paused{false}, active{false}, ampWanted{false}, ampEnabled{false};
  std::atomic<uint8_t> volume{50}, kind{TONE_NONE}, style{TONE_STYLE_BWOMP}, nextKind{TONE_NONE};
  std::atomic<uint32_t> epoch{0}, jobs{0}, dropped{0}, startedMs{0};
  std::atomic<uint32_t> writeCalls{0}, bytesWritten{0}, shortWrites{0}, failures{0};
  std::atomic<uint32_t> lastRequested{0}, lastWritten{0};
  std::atomic<int> lastError{0};
};
static ToneWorkerState toneWorker;
static QueueHandle_t toneRequestQueue = nullptr;
static TaskHandle_t toneTaskHandle = nullptr;
static uint32_t toneCodecIoFailures = 0;
static uint32_t toneRecoveries = 0;
static uint8_t toneRecoveryAttempts = 0;
static bool toneRecoveryAttempted = false;
static uint32_t toneLastRecoveryMs = 0;
static uint32_t lastLocalSentMs = 0;
static uint8_t lastLocalSentChannel = PUBLIC_CHANNEL_INDEX;
static uint32_t lastLocalSentTo = BROADCAST_ADDR;
static uint16_t unreadPublic = 0;
static uint16_t unreadFamily = 0;
static uint16_t unreadDirect = 0;
static char previewLatest[72] = "";
static char previewPublic[72] = "";
static char previewFamily[72] = "";
static char previewDirect[72] = "";

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_disp_t* display = nullptr;
static lv_color_t lvBuf1[SCREEN_W * 24];
static lv_color_t lvBuf2[SCREEN_W * 24];
static lv_color_t* mapCanvasBuf = nullptr;
static lv_color_t* mapWorkerBuf = nullptr;
static SemaphoreHandle_t sdMutex = nullptr;
static QueueHandle_t sdAppendQueue = nullptr;
static TaskHandle_t sdAppendTaskHandle = nullptr;
static QueueHandle_t mapRenderResultQueue = nullptr;
static TaskHandle_t mapRenderTaskHandle = nullptr;
static lv_obj_t* pageLauncher = nullptr;
static lv_obj_t* pageFunctions = nullptr;
static lv_obj_t* pageOperate = nullptr;
static lv_obj_t* pageDiagnose = nullptr;
static lv_obj_t* pageNodes = nullptr;
static lv_obj_t* pageNodeDetail = nullptr;
static lv_obj_t* pageMeshHealth = nullptr;
static lv_obj_t* pagePacketInspector = nullptr;
static lv_obj_t* pageTxHistory = nullptr;
static lv_obj_t* pagePublicChat = nullptr;
static lv_obj_t* pagePrivateChat = nullptr;
static lv_obj_t* pageDirectChat = nullptr;
static lv_obj_t* pageGps = nullptr;
static lv_obj_t* pageMapCoverage = nullptr;
static lv_obj_t* pageSystem = nullptr;
static lv_obj_t* pageSystemInterface = nullptr;
static lv_obj_t* pageSystemSerial = nullptr;
static lv_obj_t* pageSystemRadio = nullptr;
static lv_obj_t* pageSystemGps = nullptr;
static lv_obj_t* pageWifi = nullptr;
static lv_obj_t* pageWifiStats = nullptr;
static lv_obj_t* pageWifiLocal = nullptr;
static lv_obj_t* pageWifiScan = nullptr;
static lv_obj_t* pageBacklight = nullptr;
static lv_obj_t* pageSound = nullptr;
static lv_obj_t* pageBattery = nullptr;
static lv_obj_t* pageMessageDiag = nullptr;
static lv_obj_t* currentPage = nullptr;
static lv_obj_t* navigationStack[8] = {};
static uint8_t navigationDepth = 0;
static lv_obj_t* statusBar = nullptr;
static lv_obj_t* navBar = nullptr;
static lv_obj_t* lblStatus = nullptr;
static lv_obj_t* lblRadioStatus = nullptr;
static lv_obj_t* lblGpsStatus = nullptr;
static lv_obj_t* lblBatteryStatus = nullptr;
static lv_obj_t* lblStats = nullptr;
static lv_obj_t* lblHomeLink = nullptr;
static lv_obj_t* lblHomeMessages = nullptr;
static lv_obj_t* lblHomeMessageTitle = nullptr;
static lv_obj_t* lblHomeNodes = nullptr;
static lv_obj_t* btnNavBack = nullptr;
static lv_obj_t* btnNavHome = nullptr;
static lv_obj_t* lblHomeGps = nullptr;
static lv_obj_t* lblHomePower = nullptr;
static lv_obj_t* lblHomeLinkDetail = nullptr;
static lv_obj_t* lblHomePowerDetail = nullptr;
static lv_obj_t* lblHomeUnread = nullptr;
static lv_obj_t* homePowerBar = nullptr;
static lv_obj_t* homeLinkIcon = nullptr;
#ifdef FIRMWARE_RUN_TESTS
static lv_color_t* testScreenPixels = nullptr;
static uint32_t testScreenCapturedMs = 0;
#endif
static lv_obj_t* lblTilePublic = nullptr;
static lv_obj_t* lblTilePrivate = nullptr;
static lv_obj_t* lblMsgPublic = nullptr;
static lv_obj_t* lblMsgFamily = nullptr;
static lv_obj_t* lblMsgDirect = nullptr;
static lv_obj_t* lblPublicChatTitle = nullptr;
static lv_obj_t* lblPrivateChatTitle = nullptr;
static lv_obj_t* lblPublicChannelStats = nullptr;
static lv_obj_t* lblPrivateChannelStats = nullptr;
static lv_obj_t* lblPrivateChannelHint = nullptr;
static lv_obj_t* listNodes = nullptr;
static lv_obj_t* lblNodeListMode = nullptr;
static lv_obj_t* taNodeDetail = nullptr;
static lv_obj_t* btnNodeFavorite = nullptr;
static lv_obj_t* lblNodeFavorite = nullptr;
static lv_obj_t* lblMeshHealth = nullptr;
static lv_obj_t* taPacketInspector = nullptr;
static lv_obj_t* taTxHistory = nullptr;
static lv_obj_t* btnTxRetry = nullptr;
static lv_obj_t* lblSystemInterface = nullptr;
static lv_obj_t* lblSystemSerial = nullptr;
static lv_obj_t* lblSystemRadio = nullptr;
static lv_obj_t* lblWifiState = nullptr;
static lv_obj_t* lblWifiStats = nullptr;
static lv_obj_t* lblWifiScanStatus = nullptr;
static lv_obj_t* swWifiEnabled = nullptr;
static lv_obj_t* swWifiApMode = nullptr;
static lv_obj_t* taWifiSsid = nullptr;
static lv_obj_t* taWifiPass = nullptr;
static lv_obj_t* listWifiScan = nullptr;
static lv_obj_t* sliderBacklight = nullptr;
static lv_obj_t* lblBacklight = nullptr;
static lv_obj_t* sliderToneVolume = nullptr;
static lv_obj_t* lblToneVolume = nullptr;
static lv_obj_t* dropdownToneBoost = nullptr;
static lv_obj_t* dropdownToneMessage = nullptr;
static lv_obj_t* dropdownToneDirect = nullptr;
static lv_obj_t* swReadableText = nullptr;
static lv_obj_t* lblTextMode = nullptr;
static lv_obj_t* lblBatteryStats = nullptr;
static lv_obj_t* lblGpsStats = nullptr;
static lv_obj_t* lblMapStats = nullptr;
static lv_obj_t* lblMapZoom = nullptr;
static lv_obj_t* btnMapCoverage = nullptr;
static lv_obj_t* lblMapCoverage = nullptr;
static lv_obj_t* lblMapCoverageSummary = nullptr;
static lv_obj_t* lblMapCoverageRows[MapCoverage::levels] = {};
static lv_obj_t* btnMapZoomOut = nullptr;
static lv_obj_t* btnMapZoomIn = nullptr;
static lv_obj_t* btnMapLocate = nullptr;
static lv_obj_t* btnMapFollow = nullptr;
static lv_obj_t* lblMapFollow = nullptr;
static lv_obj_t* mapLayer = nullptr;
static lv_obj_t* lblMapNotice = nullptr;
static lv_obj_t* mapScale = nullptr;
static lv_obj_t* lblMapScale = nullptr;
static lv_obj_t* mapScaleBar = nullptr;
static lv_obj_t* mapScaleEnd = nullptr;
static lv_obj_t* mapPlot = nullptr;
static lv_obj_t* mapCanvas = nullptr;
static lv_obj_t* mapDots[MAP_DOT_COUNT];
static lv_obj_t* taPublicChat = nullptr;
static lv_obj_t* taFamilyChat = nullptr;
static lv_obj_t* taDirectChat = nullptr;
static lv_obj_t* taScreenLog = nullptr;
static lv_obj_t* taScreenNodes = nullptr;
static lv_obj_t* taPublicInput = nullptr;
static lv_obj_t* taFamilyInput = nullptr;
static lv_obj_t* taDirectTo = nullptr;
static lv_obj_t* taDirectInput = nullptr;
static lv_obj_t* dropdownMsgDiagA = nullptr;
static lv_obj_t* dropdownMsgDiagB = nullptr;
static lv_obj_t* taMsgDiagMessage = nullptr;
static lv_obj_t* lblMsgDiagStatus = nullptr;
static lv_obj_t* lblMsgDiagDetail = nullptr;
static lv_obj_t* activeChatInput = nullptr;
static lv_obj_t* keyboard = nullptr;
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* keyboardScreen = nullptr;
static lv_obj_t* keyboardPrompt = nullptr;
static lv_obj_t* keyboardText = nullptr;
static lv_obj_t* landscapeKeyboard = nullptr;
static bool landscapeKeyboardOpen = false;
static bool landscapeKeyboardSendsMessage = true;
static bool wifiLocalPageBuilt = false;
static bool wifiScanPageBuilt = false;
static uint8_t deferredWifiAction = 0;
static uint32_t deferredWifiActionMs = 0;
static uint32_t lastUiRefreshMs = 0;
static uint32_t lastSerialDiagMs = 0;
static uint32_t lastSdDiagMs = 0;
static uint32_t lastMapUiRefreshMs = 0;
static bool forcePageRefresh = true;
static bool mapNearbyMode = false;
static bool mapFocusSelectedNode = false;
static MapCenterMode mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
static MapNavigation::View mapView;
static MapCoverage::Profile mapCoverage;
static bool mapZoomSavePending = false;
static uint32_t mapZoomSaveAtMs = 0;
static bool mapDragging = false, mapDragMoved = false;
static lv_point_t mapDragStart;
static MapNavigation::Point mapDragOrigin{0, 0};
static int mapDragZoom = MAP_TILE_MAX_ZOOM;
static bool mapNavigationTesting = false;
static bool mapCanvasCached = false;
static bool mapRenderPending = false;
static bool mapTileRootFound = false;
static bool lastMapLocationValid = false;
static bool mapNodeDetailViewActive = false;
static uint32_t lastMapCacheSaveMs = 0;
static uint32_t lastMapRenderDurationMs = 0;
static uint32_t maxMapRenderDurationMs = 0;
static uint32_t lastMapRenderFinishedMs = 0;
static uint32_t mapRenderCount = 0;
static uint32_t mapRenderDraws = 0;
static uint32_t mapRenderCacheHits = 0;
static uint32_t mapRenderWorkerStarts = 0;
static uint32_t mapRenderWorkerFailures = 0;
static uint32_t mapRenderWorkerApplies = 0;
static uint32_t mapRenderWorkerBusySkips = 0;
static volatile bool mapRenderWorkerRunning = false;
static volatile bool mapRenderResultPending = false;
static bool mapRenderRetryPending = false;
static MapRenderJob mapRenderActiveJob;
static uint32_t nextMapRenderJobId = 1;
static uint32_t selectedNodeNum = 0;
static uint32_t lastNodeListRefreshMs = 0;
static uint32_t nodeListClearedMs = 0;
static NodeSortMode nodeSortMode = NODE_SORT_LAST_HEARD;
static NodeFilterMode nodeFilterMode = NODE_FILTER_ALL;
static NodeListViewMode nodeListViewMode = NODE_LIST_VIEW_COMPACT;
static int cachedMapZoom = -1;
static long cachedMapTileX = LONG_MIN;
static long cachedMapTileY = LONG_MIN;
static int cachedMapPixelX = INT_MIN;
static int cachedMapPixelY = INT_MIN;
static uint16_t cachedMapHeight = 0;
static bool cachedMapTileFound = false;
static double cachedMapLat = 0.0;
static double cachedMapLon = 0.0;
static double defaultMapLat = 0.0;
static double defaultMapLon = 0.0;
static int32_t defaultMapAlt = 0;
static char mapCacheStatus[48] = "not checked";
static char mapTileRoot[64] = "/s3-lora/tiles";
static uint32_t touchSamples = 0;
static uint32_t lastTouchMs = 0;
static uint16_t lastTouchX = 0;
static uint16_t lastTouchY = 0;
static uint32_t sdLockTimeouts = 0;
static uint32_t sdAppendQueued = 0;
static uint32_t sdAppendWritten = 0;
static uint32_t sdAppendFallbacks = 0;
static uint32_t sdAppendDropped = 0;
static uint32_t sdAppendTruncated = 0;
static constexpr uint32_t UI_DIRTY_DASHBOARD = 1UL << 0;
static constexpr uint32_t UI_DIRTY_CHAT = 1UL << 1;
static constexpr uint32_t UI_DIRTY_NODES = 1UL << 2;
static constexpr uint32_t UI_DIRTY_NODE_DETAIL = 1UL << 3;
static constexpr uint32_t UI_DIRTY_MAP = 1UL << 4;
static constexpr uint32_t UI_DIRTY_EVENT_LOG = 1UL << 5;
static constexpr uint32_t UI_DIRTY_PACKET_LOG = 1UL << 6;
static constexpr uint32_t UI_DIRTY_TX = 1UL << 7;
static constexpr uint32_t UI_DIRTY_CONFIG = 1UL << 8;
static constexpr uint32_t UI_DIRTY_SYSTEM = 1UL << 9;
static constexpr uint32_t UI_DIRTY_ALL = 0xFFFFFFFFUL;
static uint32_t uiDirtyFlags = UI_DIRTY_ALL;

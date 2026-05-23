#include <Arduino.h>
#include <limits.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <mbedtls/base64.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <TinyGPSPlus.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "constants.h"
#include "src/meshtastic/admin.pb.h"
#include "src/meshtastic/mesh.pb.h"
#include "src/meshtastic/telemetry.pb.h"
#include "src/meshtastic/portnums.pb.h"
#include "src/UIConfig.h"
#include "src/WebUi.h"

LV_FONT_DECLARE(comic_neue_12);
LV_FONT_DECLARE(comic_neue_14);

HardwareSerial SerialLoRa(2);
HardwareSerial SerialGPS(1);
WebServer server(80);
Preferences prefs;
TFT_eSPI tft;
TinyGPSPlus localGps;

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
  char text[64] = "";
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
  bool hasSerial = false;
  uint32_t lastConfigMs = 0;
  uint32_t lastModuleMs = 0;
  meshtastic_Config_LoRaConfig lora = meshtastic_Config_LoRaConfig_init_zero;
  meshtastic_Config_DeviceConfig device = meshtastic_Config_DeviceConfig_init_zero;
  meshtastic_Config_PositionConfig position = meshtastic_Config_PositionConfig_init_zero;
  meshtastic_Config_PowerConfig power = meshtastic_Config_PowerConfig_init_zero;
  meshtastic_ModuleConfig_SerialConfig serial = meshtastic_ModuleConfig_SerialConfig_init_zero;
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

enum MapCenterMode : uint8_t {
  MAP_CENTER_CURRENT_DEVICE,
  MAP_CENTER_SELECTED_NODE,
  MAP_CENTER_LAST_LOCATION
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
  bool quickStartSent = false;
  uint32_t lastGaugeMs = 0;
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
  int16_t calibrationOffsetTenths = 0;
  int16_t calibrationOffsetMv = 0;
  int percent = 100;
  int gaugePercent = 100;
  int correctedGaugePercent = 100;
  uint8_t trendSampleCount = 0;
  uint8_t stableSampleCount = 0;
  char powerState[24] = "ext power";
  char percentSource[32] = "MAX17048";
  uint16_t gaugeVersion = 0;
  uint16_t rawGaugeSoc = 0;
  float gaugeSoc = 100.0f;
  float correctedGaugeSoc = 100.0f;
  int voltagePercent = 100;
  bool charging = false;
  uint32_t previousPackMv = 0;
  uint32_t previousSampleMs = 0;
  uint32_t lastLearnSaveMs = 0;
  uint8_t trendHead = 0;
  uint8_t trendCount = 0;
  uint32_t trendMv[12] = {};
  uint32_t trendMs[12] = {};
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
static constexpr int8_t PUBLIC_CHANNEL_INDEX = 0;
static constexpr uint32_t BROADCAST_ADDR = 0xFFFFFFFFUL;
#ifndef ONE_TIME_CHANNEL_PROVISION
#define ONE_TIME_CHANNEL_PROVISION 0
#endif
static const char* CHANNEL_PROVISION_PREF_KEY = "chanProv1";
static const char* CHANNEL_PROVISION_PUBLIC_NAME = "Public";
static const char* CHANNEL_PROVISION_FAMILY_NAME = "Family";
static const char* CHANNEL_PROVISION_FAMILY_PSK = "0x66e38d5a81f3136d16d6d5cc1f872298";
static constexpr uint32_t CHANNEL_PROVISION_BOOT_DELAY_MS = 8000;
static constexpr uint32_t CHANNEL_PROVISION_RETRY_MS = 3000;
static constexpr uint32_t CHANNEL_PROVISION_STEP_MS = 700;
static constexpr uint8_t CHANNEL_PROVISION_FINAL_STEP = 10;
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
static uint32_t magic1Count = 0;
static uint32_t magic2Count = 0;
static uint32_t streamFrames = 0;
static uint32_t invalidFrameLengths = 0;
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
static uint32_t gpsBytesFromLocal = 0;
static uint32_t lastLocalGpsByteMs = 0;
static uint32_t lastLocalGpsFixLogMs = 0;
static bool localGpsHasFix = false;
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
static uint32_t lastLocalSentMs = 0;
static uint8_t lastLocalSentChannel = PUBLIC_CHANNEL_INDEX;
static uint32_t lastLocalSentTo = BROADCAST_ADDR;
static uint16_t unreadPublic = 0;
static uint16_t unreadFamily = 0;
static uint16_t unreadDirect = 0;
static char previewPublic[72] = "";
static char previewFamily[72] = "";
static char previewDirect[72] = "";

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_disp_t* display = nullptr;
static lv_color_t lvBuf1[SCREEN_W * 24];
static lv_color_t lvBuf2[SCREEN_W * 24];
static lv_color_t* mapCanvasBuf = nullptr;
static uint16_t mapReadBuf[MAP_PLOT_W];
static lv_obj_t* pageLauncher = nullptr;
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
static lv_obj_t* pageBattery = nullptr;
static lv_obj_t* currentPage = nullptr;
static lv_obj_t* previousPage = nullptr;
static lv_obj_t* statusBar = nullptr;
static lv_obj_t* navBar = nullptr;
static lv_obj_t* lblStatus = nullptr;
static lv_obj_t* lblRadioStatus = nullptr;
static lv_obj_t* lblGpsStatus = nullptr;
static lv_obj_t* lblBatteryStatus = nullptr;
static lv_obj_t* lblStats = nullptr;
static lv_obj_t* lblHomeLink = nullptr;
static lv_obj_t* lblHomeMessages = nullptr;
static lv_obj_t* lblHomeGps = nullptr;
static lv_obj_t* lblHomePower = nullptr;
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
static lv_obj_t* lblMeshHealth = nullptr;
static lv_obj_t* taPacketInspector = nullptr;
static lv_obj_t* taTxHistory = nullptr;
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
static lv_obj_t* swReadableText = nullptr;
static lv_obj_t* lblTextMode = nullptr;
static lv_obj_t* lblBatteryStats = nullptr;
static lv_obj_t* lblGpsStats = nullptr;
static lv_obj_t* lblMapStats = nullptr;
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
static bool mapExpanded = false;
static bool mapZoomedOut = false;
static bool mapCanvasCached = false;
static bool mapRenderPending = false;
static bool mapTileRootFound = false;
static bool lastMapLocationValid = false;
static bool mapNodeDetailViewActive = false;
static uint32_t lastMapCacheSaveMs = 0;
static uint32_t selectedNodeNum = 0;
static uint32_t lastNodeListRefreshMs = 0;
static uint32_t nodeListClearedMs = 0;
static NodeSortMode nodeSortMode = NODE_SORT_LAST_HEARD;
static NodeFilterMode nodeFilterMode = NODE_FILTER_ALL;
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

static void markUiDirty(uint32_t flags) {
  uiDirtyFlags |= flags;
}

static bool isUiDirty(uint32_t flags) {
  return (uiDirtyFlags & flags) != 0;
}

static void clearUiDirty(uint32_t flags) {
  uiDirtyFlags &= ~flags;
}

static bool sendTextMessage(const char* text, int8_t channelIndex = PUBLIC_CHANNEL_INDEX);
static bool sendDirectTextMessage(const char* text, uint32_t toNode);
static bool isDirectAddress(uint32_t to);
static void sendDirectFromInput(lv_obj_t* toInput, lv_obj_t* msgInput);
static const char* publicChannelLabel();
static const char* privateChannelLabel();
static void formatChannelPrompt(const char* channel, char* out, size_t outSize);
static void refreshChannelUiLabels();
static const char* nodeName(uint32_t num);
static NodeRecord* findOrCreateNode(uint32_t num);
static void allocateRuntimeBuffers();
static void appendLine(char* buffer, size_t bufferSize, const char* line);
static void appendPacketEvent(const char* line);
static void refreshDashboardLabels();
static void showNodeDetail(uint32_t nodeNum);
static void directSelectedNode();
static void showSelectedNodeOnMap();
static void cycleNodeSort();
static void cycleNodeFilter();
static void clearNodeList();
static bool sendHeltecNodeDbReset();
static bool retryLastTx();
static void refreshTxHistoryView();
static void refreshNodeList(bool force = false);
static void refreshMapUi();
static double distanceMeters(double lat1, double lon1, double lat2, double lon2);
static void loadMapCacheFromSd();
static void printSdTileSummary();
static void printSdTileFastSummary();
static void refreshChatViews(bool force = false);
static void setMapNearbyMode(bool enabled);
static void setReadableTextMode(bool enabled, bool save);
static void rebuildScreenUi(lv_obj_t* targetPage = nullptr);
static void styleDarkObject(lv_obj_t* obj, uint32_t bg, uint32_t text = COLOR_TEXT);
static void styleDarkBorder(lv_obj_t* obj, uint32_t color = COLOR_BORDER);
static void styleBoundedLabel(lv_obj_t* label, lv_coord_t width, uint32_t color = COLOR_TEXT, lv_label_long_mode_t mode = LV_LABEL_LONG_DOT);
static void showPage(lv_obj_t* target, bool remember = true);
static void ensureWifiScanPage();
static void detectMapTileRoot(bool logResult = false);
static void printMapTileRootCandidates();
static void serviceOneTimeChannelProvision();

static void* allocatePsramBuffer(size_t bytes, const char* name) {
  void* ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  const char* source = "PSRAM";
  if (!ptr) {
    ptr = heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    source = "internal";
  }
  if (ptr) {
    memset(ptr, 0, bytes);
    Serial.printf("[boot] %s buffer %u bytes in %s\n", name, (unsigned)bytes, source);
  } else {
    Serial.printf("[boot] %s buffer allocation failed (%u bytes)\n", name, (unsigned)bytes);
  }
  return ptr;
}

static void allocateRuntimeBuffers() {
  if (!eventLog) eventLog = (char*)allocatePsramBuffer(LOG_SIZE, "eventLog");
  if (!packetLog) packetLog = (char*)allocatePsramBuffer(PACKET_LOG_SIZE, "packetLog");
  if (!publicChatLog) publicChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "publicChat");
  if (!familyChatLog) familyChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "familyChat");
  if (!directChatLog) directChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "directChat");
  if (!mapCanvasBuf) {
    mapCanvasBuf = (lv_color_t*)allocatePsramBuffer(MAP_CANVAS_BYTES, "mapCanvas");
  }
}

static size_t loadSdTextTail(const char* path, char* buffer, size_t bufferSize) {
  if (!sdStorage.available || !path || !buffer || bufferSize == 0 || !SD_MMC.exists(path)) return 0;
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) return 0;
  size_t fileSize = file.size();
  size_t maxRead = bufferSize - 1;
  size_t start = fileSize > maxRead ? fileSize - maxRead : 0;
  if (start > 0) file.seek(start);
  size_t bytesRead = file.read((uint8_t*)buffer, min(maxRead, fileSize - start));
  file.close();
  buffer[bytesRead] = '\0';

  if (start > 0) {
    char* firstNewline = strchr(buffer, '\n');
    if (firstNewline && firstNewline[1]) {
      memmove(buffer, firstNewline + 1, strlen(firstNewline + 1) + 1);
    }
  }
  return strlen(buffer);
}

static bool chatLineLooksValid(const char* line) {
  if (!line) return false;
  while (*line == '\r' || *line == '\n' || *line == ' ') line++;
  if (!*line) return false;
  const char* newline = strchr(line, '\n');
  size_t lineLen = newline ? (size_t)(newline - line) : strlen(line);
  if (line[0] == '[' && strstr(line, "] [")) return true;
  if (lineLen < 8) return false;
  bool hasText = false;
  size_t printable = 0;
  for (size_t i = 0; i < lineLen && i < 160; i++) {
    unsigned char c = (unsigned char)line[i];
    if (c >= 0x20 && c < 0x7F) printable++;
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasText = true;
  }
  return hasText && printable >= min<size_t>(lineLen, 8);
}

static void sanitizeChatLog(char* buffer) {
  if (!buffer || !buffer[0]) return;
  char* line = buffer;
  while (line && line[0]) {
    while (*line == '\r' || *line == '\n') line++;
    if (chatLineLooksValid(line)) {
      if (line != buffer) memmove(buffer, line, strlen(line) + 1);
      return;
    }
    line = strchr(line, '\n');
    if (line) line++;
  }
  buffer[0] = '\0';
}

static void loadChatLogsFromSd() {
  publicChatLog[0] = '\0';
  familyChatLog[0] = '\0';
  directChatLog[0] = '\0';
  loadSdTextTail(SD_PUBLIC_CHAT_PATH, publicChatLog, CHAT_SIZE);
  loadSdTextTail(SD_SECONDARY_CHAT_PATH, familyChatLog, CHAT_SIZE);
  if (!familyChatLog[0]) loadSdTextTail(SD_LEGACY_SECONDARY_CHAT_PATH, familyChatLog, CHAT_SIZE);
  loadSdTextTail(SD_DIRECT_CHAT_PATH, directChatLog, CHAT_SIZE);
  sanitizeChatLog(publicChatLog);
  sanitizeChatLog(familyChatLog);
  sanitizeChatLog(directChatLog);
  publicChatHistoryBytes = strlen(publicChatLog);
  familyChatHistoryBytes = strlen(familyChatLog);
  directChatHistoryBytes = strlen(directChatLog);
  chatHistoryLoadedFromSd = publicChatHistoryBytes || familyChatHistoryBytes || directChatHistoryBytes;
  char line[112];
  snprintf(line, sizeof(line), "[sd] chat history loaded public=%u secondary=%u direct=%u\n",
           (unsigned)publicChatHistoryBytes,
           (unsigned)familyChatHistoryBytes,
           (unsigned)directChatHistoryBytes);
  appendLine(eventLog, LOG_SIZE, line);
  refreshChatViews();
}

static bool appendSdLine(const char* path, const char* line) {
  if (!sdStorage.available || !path || !line) return false;
  File file = SD_MMC.open(path, FILE_APPEND);
  if (!file) {
    sdStorage.writeErrors++;
    strlcpy(sdStorage.status, "open failed", sizeof(sdStorage.status));
    return false;
  }
  size_t len = strlen(line);
  bool ok = file.write((const uint8_t*)line, len) == len;
  file.close();
  if (ok) {
    sdStorage.writes++;
    strlcpy(sdStorage.status, "logging", sizeof(sdStorage.status));
  } else {
    sdStorage.writeErrors++;
    strlcpy(sdStorage.status, "write failed", sizeof(sdStorage.status));
  }
  return ok;
}

static void initSdStorage() {
  mapTileRootFound = false;
  strlcpy(mapTileRoot, DEFAULT_MAP_TILE_ROOT, sizeof(mapTileRoot));
  Serial.printf("[sd] setPins clk=%d cmd=%d d0=%d d1=%d d2=%d d3=%d oneBit=%s freq=%d\n",
                SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN, SD_D1_PIN, SD_D2_PIN, SD_D3_PIN,
                SD_MMC_ONE_BIT ? "true" : "false", SD_MMC_FREQ);
  SD_MMC.end();
  bool pinsOk = SD_MMC_ONE_BIT
                  ? SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN)
                  : SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN, SD_D1_PIN, SD_D2_PIN, SD_D3_PIN);
  if (!pinsOk) {
    sdStorage.available = false;
    sdStorage.cardSizeBytes = 0;
    sdStorage.totalBytes = 0;
    sdStorage.usedBytes = 0;
    strlcpy(sdStorage.cardType, "none", sizeof(sdStorage.cardType));
    strlcpy(sdStorage.status, "pin setup failed", sizeof(sdStorage.status));
    Serial.println("[sd] setPins failed");
    return;
  }

  if (!SD_MMC.begin("/sdcard", SD_MMC_ONE_BIT, false, SD_MMC_FREQ)) {
    sdStorage.available = false;
    sdStorage.cardSizeBytes = 0;
    sdStorage.totalBytes = 0;
    sdStorage.usedBytes = 0;
    strlcpy(sdStorage.cardType, "none", sizeof(sdStorage.cardType));
    strlcpy(sdStorage.status, "mount failed", sizeof(sdStorage.status));
    Serial.println("[sd] mount failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    sdStorage.available = false;
    sdStorage.cardSizeBytes = 0;
    sdStorage.totalBytes = 0;
    sdStorage.usedBytes = 0;
    strlcpy(sdStorage.cardType, "none", sizeof(sdStorage.cardType));
    strlcpy(sdStorage.status, "card none", sizeof(sdStorage.status));
    Serial.println("[sd] card type none");
    SD_MMC.end();
    return;
  }

  if (cardType == CARD_MMC) strlcpy(sdStorage.cardType, "MMC", sizeof(sdStorage.cardType));
  else if (cardType == CARD_SD) strlcpy(sdStorage.cardType, "SDSC", sizeof(sdStorage.cardType));
  else if (cardType == CARD_SDHC) strlcpy(sdStorage.cardType, "SDHC", sizeof(sdStorage.cardType));
  else strlcpy(sdStorage.cardType, "unknown", sizeof(sdStorage.cardType));

  sdStorage.available = true;
  sdStorage.cardSizeBytes = SD_MMC.cardSize();
  sdStorage.totalBytes = SD_MMC.totalBytes();
  if (sdStorage.totalBytes == 0) sdStorage.totalBytes = sdStorage.cardSizeBytes;
  sdStorage.usedBytes = SD_MMC.usedBytes();
  strlcpy(sdStorage.status, "mounted", sizeof(sdStorage.status));
  Serial.printf("[sd] mounted type=%s card=%llu total=%llu used=%llu\n",
                sdStorage.cardType,
                (unsigned long long)sdStorage.cardSizeBytes,
                (unsigned long long)sdStorage.totalBytes,
                (unsigned long long)sdStorage.usedBytes);
  if (!SD_MMC.exists(SD_DIR)) SD_MMC.mkdir(SD_DIR);
  if (!SD_MMC.exists(SD_POSITIONS_PATH)) {
    appendSdLine(SD_POSITIONS_PATH, "uptime_ms,node,name,source,lat,lon,alt_m\n");
  }
  detectMapTileRoot(true);
  loadChatLogsFromSd();
  loadMapCacheFromSd();
}

static void refreshSdUsage() {
  if (!sdStorage.available) return;
  // Some ESP32-S3 SD_MMC/card combinations can block inside usedBytes() after
  // files have been opened. Keep the mount-time usage values so UI refreshes
  // and diagnostics cannot stall the display loop.
}

static void buildTileRootPath(const char* root, int zoom, char* path, size_t pathSize) {
  if (!path || pathSize == 0) return;
  if (!root || !root[0] || strcmp(root, "/") == 0) snprintf(path, pathSize, "/%d", zoom);
  else snprintf(path, pathSize, "%s/%d", root, zoom);
}

static void buildTilePath(const char* root, int zoom, long x, long y, char* path, size_t pathSize) {
  if (!path || pathSize == 0) return;
  if (!root || !root[0] || strcmp(root, "/") == 0) snprintf(path, pathSize, "/%d/%ld/%ld.rgb565", zoom, x, y);
  else snprintf(path, pathSize, "%s/%d/%ld/%ld.rgb565", root, zoom, x, y);
}

static uint8_t scoreMapTileRoot(const char* root) {
  if (!sdStorage.available || !root || !root[0]) return 0;
  uint8_t score = 0;
  for (int zoom = MAP_TILE_MIN_ZOOM; zoom <= MAP_TILE_MAX_ZOOM; zoom++) {
    char zoomPath[64];
    buildTileRootPath(root, zoom, zoomPath, sizeof(zoomPath));
    if (!SD_MMC.exists(zoomPath)) continue;
    File zoomDir = SD_MMC.open(zoomPath);
    if (!zoomDir || !zoomDir.isDirectory()) {
      if (zoomDir) zoomDir.close();
      continue;
    }
    score += 10;
    File child = zoomDir.openNextFile();
    while (child) {
      bool hasChildDir = child.isDirectory();
      child.close();
      if (hasChildDir) {
        score += 2;
        break;
      }
      child = zoomDir.openNextFile();
    }
    zoomDir.close();
  }
  return score;
}

static void detectMapTileRoot(bool logResult) {
  static const char* candidates[] = {
    "/s3-lora/tiles",
    "/tiles",
    "/map_tiles",
    "/maps",
    "/maps/tiles",
    "/s3-lora/s3-lora/tiles",
    "/sdcard/s3-lora/tiles",
    "/"
  };
  uint8_t bestScore = 0;
  const char* bestRoot = DEFAULT_MAP_TILE_ROOT;
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    uint8_t score = scoreMapTileRoot(candidates[i]);
    if (score > bestScore) {
      bestScore = score;
      bestRoot = candidates[i];
    }
  }
  strlcpy(mapTileRoot, bestRoot, sizeof(mapTileRoot));
  mapTileRootFound = bestScore > 0;
  if (logResult) {
    Serial.printf("[tiles] selectedRoot=%s found=%s score=%u\n",
                  mapTileRoot,
                  mapTileRootFound ? "true" : "false",
                  (unsigned)bestScore);
  }
}

static void printMapTileRootCandidates() {
  static const char* candidates[] = {
    "/s3-lora/tiles",
    "/tiles",
    "/map_tiles",
    "/maps",
    "/maps/tiles",
    "/s3-lora/s3-lora/tiles",
    "/sdcard/s3-lora/tiles",
    "/"
  };
  Serial.println("[tileroots] begin");
  if (!sdStorage.available) {
    Serial.printf("[tileroots] SD unavailable: %s\n", sdStorage.status);
    Serial.println("[tileroots] end");
    return;
  }
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    Serial.printf("[tileroots] %s score=%u\n", candidates[i], (unsigned)scoreMapTileRoot(candidates[i]));
  }
  Serial.printf("[tileroots] selected=%s found=%s\n", mapTileRoot, mapTileRootFound ? "true" : "false");
  Serial.println("[tileroots] end");
}

static const char* pathBasename(const char* path) {
  if (!path) return "";
  const char* slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

static long parseTileNumber(const char* text) {
  if (!text || !text[0]) return -1;
  char temp[24];
  strlcpy(temp, pathBasename(text), sizeof(temp));
  char* dot = strchr(temp, '.');
  if (dot) *dot = '\0';
  char* end = nullptr;
  long value = strtol(temp, &end, 10);
  return end == temp ? -1 : value;
}

static void printSdTileSummary() {
  Serial.println("[tiles] begin");
  if (!sdStorage.available) {
    Serial.printf("[tiles] SD unavailable: %s\n", sdStorage.status);
    Serial.println("[tiles] end");
    return;
  }
  detectMapTileRoot(true);
  Serial.printf("[tiles] card=%s total=%llu used=%llu root=%s found=%s\n",
                sdStorage.cardType,
                (unsigned long long)sdStorage.totalBytes,
                (unsigned long long)sdStorage.usedBytes,
                mapTileRoot,
                mapTileRootFound ? "true" : "false");
  for (int zoom = MAP_TILE_MIN_ZOOM; zoom <= MAP_TILE_MAX_ZOOM; zoom++) {
    char zoomPath[64];
    buildTileRootPath(mapTileRoot, zoom, zoomPath, sizeof(zoomPath));
    if (!SD_MMC.exists(zoomPath)) {
      Serial.printf("[tiles] z%d missing\n", zoom);
      continue;
    }
    File zoomDir = SD_MMC.open(zoomPath);
    if (!zoomDir || !zoomDir.isDirectory()) {
      Serial.printf("[tiles] z%d not a directory\n", zoom);
      if (zoomDir) zoomDir.close();
      continue;
    }

    uint32_t xDirs = 0;
    uint32_t tileCount = 0;
    uint64_t totalBytes = 0;
    uint32_t badSize = 0;
    long minX = LONG_MAX;
    long maxX = LONG_MIN;
    long minY = LONG_MAX;
    long maxY = LONG_MIN;

    File xDir = zoomDir.openNextFile();
    while (xDir) {
      if (xDir.isDirectory()) {
        long x = parseTileNumber(xDir.name());
        if (x >= 0) {
          xDirs++;
          if (xDirs % 25 == 0) {
            Serial.printf("[tiles] z%d scanning xdirs=%lu tiles=%lu\n",
                          zoom,
                          (unsigned long)xDirs,
                          (unsigned long)tileCount);
          }
          if (x < minX) minX = x;
          if (x > maxX) maxX = x;
        }
        File tile = xDir.openNextFile();
        while (tile) {
          if (!tile.isDirectory()) {
            const char* name = tile.name();
            size_t len = strlen(name);
            if (len >= 7 && strcmp(name + len - 7, ".rgb565") == 0) {
              long y = parseTileNumber(name);
              if (y >= 0) {
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
              }
              size_t tileSize = tile.size();
              totalBytes += tileSize;
              tileCount++;
              if (tileSize != MAP_TILE_SIZE * MAP_TILE_SIZE * 2) badSize++;
            }
          }
          tile.close();
          delay(0);
          tile = xDir.openNextFile();
        }
      }
      xDir.close();
      delay(1);
      xDir = zoomDir.openNextFile();
    }
    zoomDir.close();

    if (tileCount == 0) {
      Serial.printf("[tiles] z%d empty dirs=%lu\n", zoom, (unsigned long)xDirs);
    } else {
      Serial.printf("[tiles] z%d tiles=%lu xdirs=%lu x=%ld..%ld y=%ld..%ld bytes=%llu badSize=%lu\n",
                    zoom,
                    (unsigned long)tileCount,
                    (unsigned long)xDirs,
                    minX,
                    maxX,
                    minY,
                    maxY,
                    (unsigned long long)totalBytes,
                    (unsigned long)badSize);
    }
  }
  Serial.println("[tiles] end");
}

static void inspectTileYRange(File& xDir, uint32_t* count, long* minY, long* maxY) {
  if (count) *count = 0;
  if (minY) *minY = LONG_MAX;
  if (maxY) *maxY = LONG_MIN;
  File tile = xDir.openNextFile();
  while (tile) {
    if (!tile.isDirectory()) {
      const char* name = tile.name();
      size_t len = strlen(name);
      if (len >= 7 && strcmp(name + len - 7, ".rgb565") == 0) {
        long y = parseTileNumber(name);
        if (y >= 0) {
          if (count) (*count)++;
          if (minY && y < *minY) *minY = y;
          if (maxY && y > *maxY) *maxY = y;
        }
      }
    }
    tile.close();
    tile = xDir.openNextFile();
  }
}

static void printSdTileFastSummary() {
  Serial.println("[tilesfast] begin");
  if (!sdStorage.available) {
    Serial.printf("[tilesfast] SD unavailable: %s\n", sdStorage.status);
    Serial.println("[tilesfast] end");
    return;
  }
  detectMapTileRoot(true);
  Serial.printf("[tilesfast] card=%s total=%llu used=%llu root=%s found=%s\n",
                sdStorage.cardType,
                (unsigned long long)sdStorage.totalBytes,
                (unsigned long long)sdStorage.usedBytes,
                mapTileRoot,
                mapTileRootFound ? "true" : "false");
  for (int zoom = MAP_TILE_MIN_ZOOM; zoom <= MAP_TILE_MAX_ZOOM; zoom++) {
    char zoomPath[64];
    buildTileRootPath(mapTileRoot, zoom, zoomPath, sizeof(zoomPath));
    if (!SD_MMC.exists(zoomPath)) {
      Serial.printf("[tilesfast] z%d missing\n", zoom);
      continue;
    }
    File zoomDir = SD_MMC.open(zoomPath);
    if (!zoomDir || !zoomDir.isDirectory()) {
      Serial.printf("[tilesfast] z%d not a directory\n", zoom);
      if (zoomDir) zoomDir.close();
      continue;
    }

    uint32_t xDirs = 0;
    long minX = LONG_MAX;
    long maxX = LONG_MIN;
    char sampleMinXPath[48] = "";
    char sampleMaxXPath[48] = "";
    File xDir = zoomDir.openNextFile();
    while (xDir) {
      if (xDir.isDirectory()) {
        long x = parseTileNumber(xDir.name());
        if (x >= 0) {
          xDirs++;
          if (x < minX) {
            minX = x;
            strlcpy(sampleMinXPath, xDir.name(), sizeof(sampleMinXPath));
          }
          if (x > maxX) {
            maxX = x;
            strlcpy(sampleMaxXPath, xDir.name(), sizeof(sampleMaxXPath));
          }
        }
      }
      xDir.close();
      xDir = zoomDir.openNextFile();
    }
    zoomDir.close();

    uint32_t minXTileCount = 0;
    uint32_t maxXTileCount = 0;
    long minXMinY = LONG_MAX;
    long minXMaxY = LONG_MIN;
    long maxXMinY = LONG_MAX;
    long maxXMaxY = LONG_MIN;
    if (sampleMinXPath[0]) {
      File sample = SD_MMC.open(sampleMinXPath);
      if (sample && sample.isDirectory()) inspectTileYRange(sample, &minXTileCount, &minXMinY, &minXMaxY);
      if (sample) sample.close();
    }
    if (sampleMaxXPath[0] && strcmp(sampleMaxXPath, sampleMinXPath) != 0) {
      File sample = SD_MMC.open(sampleMaxXPath);
      if (sample && sample.isDirectory()) inspectTileYRange(sample, &maxXTileCount, &maxXMinY, &maxXMaxY);
      if (sample) sample.close();
    }

    Serial.printf("[tilesfast] z%d xdirs=%lu x=%ld..%ld sampleMinX=%lu y=%ld..%ld sampleMaxX=%lu y=%ld..%ld\n",
                  zoom,
                  (unsigned long)xDirs,
                  minX,
                  maxX,
                  (unsigned long)minXTileCount,
                  minXTileCount ? minXMinY : -1,
                  minXTileCount ? minXMaxY : -1,
                  (unsigned long)maxXTileCount,
                  maxXTileCount ? maxXMinY : -1,
                  maxXTileCount ? maxXMaxY : -1);
  }
  Serial.println("[tilesfast] end");
}

static void logPositionToSd(uint32_t from, const char* sourceKind, double lat, double lon, int32_t alt) {
  if (!sdStorage.available) return;
  char line[180];
  snprintf(line, sizeof(line), "%lu,!%08lX,%s,%s,%.6f,%.6f,%ld\n",
           (unsigned long)millis(),
           (unsigned long)from,
           nodeName(from),
           sourceKind ? sourceKind : "unknown",
           lat,
           lon,
           (long)alt);
  appendSdLine(SD_POSITIONS_PATH, line);
}

static void saveLastLocationToSd(double lat, double lon, int32_t alt) {
  defaultMapLat = lat;
  defaultMapLon = lon;
  defaultMapAlt = alt;
  lastMapLocationValid = true;
  if (!sdStorage.available) return;
  if (SD_MMC.exists(SD_LAST_LOCATION_PATH)) SD_MMC.remove(SD_LAST_LOCATION_PATH);
  File file = SD_MMC.open(SD_LAST_LOCATION_PATH, FILE_WRITE);
  if (!file) return;
  file.printf("%.7f,%.7f,%ld,%lu\n", lat, lon, (long)alt, (unsigned long)millis());
  file.close();
}

static bool loadLastLocationFromSd() {
  if (!sdStorage.available || !SD_MMC.exists(SD_LAST_LOCATION_PATH)) return false;
  File file = SD_MMC.open(SD_LAST_LOCATION_PATH, FILE_READ);
  if (!file) return false;
  String line = file.readStringUntil('\n');
  file.close();
  double lat = 0.0;
  double lon = 0.0;
  long alt = 0;
  if (sscanf(line.c_str(), "%lf,%lf,%ld", &lat, &lon, &alt) != 3) return false;
  if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) return false;
  defaultMapLat = lat;
  defaultMapLon = lon;
  defaultMapAlt = alt;
  lastMapLocationValid = true;
  return true;
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

static unsigned long bytesToWholeKb(uint64_t bytes) {
  return (unsigned long)((bytes + 1023ULL) / 1024ULL);
}

static unsigned long bytesToWholeMb(uint64_t bytes) {
  return (unsigned long)((bytes + (1024ULL * 1024ULL - 1ULL)) / (1024ULL * 1024ULL));
}

static bool max17048ReadReg16(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MAX17048_ADDR, (uint8_t)2) != 2) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  value = ((uint16_t)msb << 8) | lsb;
  return true;
}

static bool max17048WriteReg16(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  return Wire.endTransmission() == 0;
}

static bool readMax17048ChargeRate(float& percentPerHour) {
  uint16_t rawCrate = 0;
  if (!max17048ReadReg16(0x16, rawCrate)) return false;
  percentPerHour = (float)((int16_t)rawCrate) * 0.208f;
  return true;
}

static int estimateLiPoPercentFromMv(uint32_t mv) {
  if (mv >= 4200) return 100;
  if (mv >= 4100) return map(mv, 4100, 4200, 90, 100);
  if (mv >= 4000) return map(mv, 4000, 4100, 78, 90);
  if (mv >= 3900) return map(mv, 3900, 4000, 62, 78);
  if (mv >= 3800) return map(mv, 3800, 3900, 45, 62);
  if (mv >= 3700) return map(mv, 3700, 3800, 28, 45);
  if (mv >= 3600) return map(mv, 3600, 3700, 12, 28);
  if (mv >= 3300) return map(mv, 3300, 3600, 0, 12);
  return 0;
}

static void updateBatteryTrend(uint32_t packMv, uint32_t nowMs) {
  static constexpr uint8_t TREND_WINDOW = sizeof(localBattery.trendMv) / sizeof(localBattery.trendMv[0]);
  localBattery.trendMv[localBattery.trendHead] = packMv;
  localBattery.trendMs[localBattery.trendHead] = nowMs;
  localBattery.trendHead = (localBattery.trendHead + 1) % TREND_WINDOW;
  if (localBattery.trendCount < TREND_WINDOW) localBattery.trendCount++;
  localBattery.trendSampleCount = localBattery.trendCount;

  if (localBattery.trendCount < 4) {
    localBattery.deltaMvPerMin = 0;
    localBattery.deltaMvPerMinTenths = 0;
    return;
  }

  uint8_t newestIndex = (localBattery.trendHead + TREND_WINDOW - 1) % TREND_WINDOW;
  uint8_t oldestIndex = (localBattery.trendHead + TREND_WINDOW - localBattery.trendCount) % TREND_WINDOW;
  uint32_t newestMs = localBattery.trendMs[newestIndex];
  uint32_t oldestMs = localBattery.trendMs[oldestIndex];
  if (newestMs <= oldestMs) return;

  uint32_t windowMs = newestMs - oldestMs;
  int32_t deltaMv = (int32_t)localBattery.trendMv[newestIndex] - (int32_t)localBattery.trendMv[oldestIndex];
  if (windowMs < 20000 || abs(deltaMv) < 3) {
    localBattery.deltaMvPerMin = 0;
    localBattery.deltaMvPerMinTenths = 0;
    return;
  }

  localBattery.deltaMvPerMinTenths = (int32_t)((int64_t)deltaMv * 600000LL / (int64_t)windowMs);
  localBattery.deltaMvPerMin = localBattery.deltaMvPerMinTenths / 10;
}

static bool decideBatteryCharging(bool chargeRateValid, float chargeRate, uint32_t packMv, uint32_t nowMs) {
  int32_t deltaMv = 0;
  uint32_t elapsedMs = 0;
  if (localBattery.previousSampleMs) {
    deltaMv = (int32_t)packMv - (int32_t)localBattery.previousPackMv;
    elapsedMs = nowMs - localBattery.previousSampleMs;
  }
  localBattery.instantDeltaMv = deltaMv;
  localBattery.previousPackMv = packMv;
  localBattery.previousSampleMs = nowMs;

  if (!chargeRateValid) return localBattery.charging;
  if (chargeRate >= 1.0f) return true;
  if (chargeRate <= 0.0f) return false;

  bool fastVoltageDrop = elapsedMs > 0 && elapsedMs <= 4000 && deltaMv <= -5;
  if (localBattery.charging && fastVoltageDrop) return false;
  if (!localBattery.charging && chargeRate >= 0.4f) return true;
  return localBattery.charging;
}

static int chooseDisplayedBatteryPercent(float soc, int voltagePercent, bool socReliable, bool charging) {
  localBattery.gaugePercent = constrain((int)roundf(soc), 0, 100);
  float correctedSoc = soc + ((float)localBattery.calibrationOffsetTenths / 10.0f);
  if (correctedSoc < 0.0f) correctedSoc = 0.0f;
  if (correctedSoc > 100.0f) correctedSoc = 100.0f;
  localBattery.correctedGaugeSoc = correctedSoc;
  localBattery.correctedGaugePercent = constrain((int)roundf(correctedSoc), 0, 100);
  localBattery.calibrationOffsetMv = 0;

  if (!socReliable) {
    strlcpy(localBattery.percentSource, "code voltage estimate", sizeof(localBattery.percentSource));
    return voltagePercent;
  }

  int disagreement = abs(localBattery.correctedGaugePercent - voltagePercent);
  if (!charging && disagreement >= 35) {
    strlcpy(localBattery.percentSource, "code sanity estimate", sizeof(localBattery.percentSource));
    return voltagePercent;
  }

  if (!charging && disagreement >= 20) {
    strlcpy(localBattery.percentSource, "MAX17048 sanity blend", sizeof(localBattery.percentSource));
    return constrain((localBattery.correctedGaugePercent * 3 + voltagePercent) / 4, 0, 100);
  }

  strlcpy(localBattery.percentSource,
          localBattery.calibrationOffsetTenths ? "MAX17048 + learned trim" : "MAX17048",
          sizeof(localBattery.percentSource));
  return localBattery.correctedGaugePercent;
}

static void updateBatteryLearning(float soc, int voltagePercent, bool socReliable, bool charging,
                                  bool chargeRateValid, float chargeRate, uint32_t nowMs) {
  if (!socReliable || charging) {
    localBattery.stableSampleCount = 0;
    return;
  }

  if (chargeRateValid && fabsf(chargeRate) > 1.5f) {
    localBattery.stableSampleCount = 0;
    return;
  }

  if (abs(localBattery.instantDeltaMv) > 3) {
    localBattery.stableSampleCount = 0;
    return;
  }

  int targetOffsetTenths = (int)roundf(((float)voltagePercent - soc) * 10.0f);
  targetOffsetTenths = constrain(targetOffsetTenths, -120, 120);
  int errorTenths = targetOffsetTenths - localBattery.calibrationOffsetTenths;
  if (abs(errorTenths) < 10) {
    localBattery.stableSampleCount = min<uint8_t>(localBattery.stableSampleCount + 1, 255);
    return;
  }

  localBattery.stableSampleCount = min<uint8_t>(localBattery.stableSampleCount + 1, 255);
  if (localBattery.stableSampleCount < 30) return;

  localBattery.calibrationOffsetTenths += errorTenths > 0 ? 1 : -1;
  localBattery.calibrationOffsetTenths = constrain(localBattery.calibrationOffsetTenths, -120, 120);
  localBattery.stableSampleCount = 0;

  if (!localBattery.lastLearnSaveMs || nowMs - localBattery.lastLearnSaveMs > 60000) {
    prefs.putShort("batTrim10", localBattery.calibrationOffsetTenths);
    localBattery.lastLearnSaveMs = nowMs;
  }
}

static bool sampleMax17048() {
  uint16_t rawVcell = 0;
  uint16_t rawSoc = 0;
  if (!max17048ReadReg16(0x02, rawVcell) || !max17048ReadReg16(0x04, rawSoc)) {
    localBattery.gaugePresent = false;
    strlcpy(localBattery.powerState, "gauge missing", sizeof(localBattery.powerState));
    return false;
  }

  uint16_t rawVersion = 0;
  if (max17048ReadReg16(0x08, rawVersion)) localBattery.gaugeVersion = rawVersion;

  uint32_t packMv = (uint32_t)(((rawVcell >> 4) * 125UL + 50UL) / 100UL);
  float soc = (float)(rawSoc >> 8) + ((float)(rawSoc & 0xFF) / 256.0f);
  if (soc < 0.0f) soc = 0.0f;
  if (soc > 100.0f) soc = 100.0f;
  int voltagePercent = estimateLiPoPercentFromMv(packMv);
  float chargeRate = 0.0f;
  bool chargeRateValid = readMax17048ChargeRate(chargeRate);
  bool socReliable = !(soc < 1.0f && packMv > 3350);
  if (!socReliable && !localBattery.quickStartSent) {
    if (max17048WriteReg16(0x06, 0x4000)) {
      localBattery.quickStartSent = true;
      appendLine(eventLog, LOG_SIZE, "[battery] MAX17048 quick-start sent\n");
    }
  }

  uint32_t now = millis();
  bool charging = decideBatteryCharging(chargeRateValid, chargeRate, packMv, now);
  updateBatteryLearning(soc, voltagePercent, socReliable, charging, chargeRateValid, chargeRate, now);
  int displayedPercent = chooseDisplayedBatteryPercent(soc, voltagePercent, socReliable, charging);
  localBattery.gaugePresent = true;
  localBattery.lastGaugeMs = now;
  localBattery.rawMv = packMv;
  localBattery.rawPackMv = packMv;
  localBattery.filteredPackMv = packMv;
  localBattery.batteryMv = packMv;
  localBattery.learnedBatteryMv = packMv;
  localBattery.rawGaugeSoc = rawSoc;
  localBattery.gaugeSoc = soc;
  localBattery.gaugeSocReliable = socReliable;
  localBattery.voltagePercent = voltagePercent;
  localBattery.chargeRateValid = chargeRateValid;
  localBattery.chargeRatePercentHr = chargeRateValid ? chargeRate : 0.0f;
  localBattery.charging = charging;
  localBattery.percent = displayedPercent;
  updateBatteryTrend(packMv, now);
  strlcpy(localBattery.powerState, localBattery.percentSource, sizeof(localBattery.powerState));
  return true;
}

static void sampleLocalBattery() {
  if (localBattery.lastGaugeMs && millis() - localBattery.lastGaugeMs < 1000) return;
  sampleMax17048();
}

static void appendLine(char* buffer, size_t bufferSize, const char* line) {
  if (!buffer || !line || bufferSize == 0) return;
  size_t used = strlen(buffer);
  size_t incoming = strlen(line);
  if (used + incoming + 1 >= bufferSize) {
    const char* marker = "--- log trimmed ---\n";
    size_t markerLen = strlen(marker);
    size_t keep = bufferSize / 2;
    memmove(buffer, buffer + used - keep, keep);
    memmove(buffer + markerLen, buffer, keep);
    memcpy(buffer, marker, markerLen);
    buffer[markerLen + keep] = '\0';
    used = strlen(buffer);
  }
  strncat(buffer, line, bufferSize - used - 1);

  if (buffer == eventLog) {
    appendSdLine(SD_EVENTS_PATH, line);
    markUiDirty(UI_DIRTY_EVENT_LOG | UI_DIRTY_DASHBOARD);
  } else if (buffer == packetLog) {
    markUiDirty(UI_DIRTY_PACKET_LOG | UI_DIRTY_SYSTEM);
  } else if (buffer == publicChatLog) {
    appendSdLine(SD_PUBLIC_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  } else if (buffer == familyChatLog) {
    appendSdLine(SD_SECONDARY_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  } else if (buffer == directChatLog) {
    appendSdLine(SD_DIRECT_CHAT_PATH, line);
    markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
  }
}

static void appendPacketEvent(const char* line) {
  appendLine(packetLog, PACKET_LOG_SIZE, line);
}

static void copyPreferenceString(const char* key, char* dest, size_t destSize, const char* fallback) {
  String value = prefs.getString(key, fallback);
  value.trim();
  if (!value.length()) value = fallback;
  strlcpy(dest, value.c_str(), destSize);
}

static void loadInterfaceSettings() {
  bool needsSeed = !prefs.isKey("devName");
  copyPreferenceString("devName", interfaceDeviceName, sizeof(interfaceDeviceName), DEVICE_NAME);
  copyPreferenceString("hostname", interfaceHostname, sizeof(interfaceHostname), DEVICE_HOSTNAME);
  copyPreferenceString("apSsid", interfaceApSsid, sizeof(interfaceApSsid), INTERFACE_AP_SSID);
  copyPreferenceString("apPass", interfaceApPass, sizeof(interfaceApPass), INTERFACE_AP_PASS);
  copyPreferenceString("webUser", webUiUser, sizeof(webUiUser), WEBUI_AUTH_USER);
  copyPreferenceString("webPass", webUiPass, sizeof(webUiPass), WEBUI_AUTH_PASS);
  interfaceApChannel = prefs.getUChar("apChannel", WIFI_AP_CHANNEL);
  if (interfaceApChannel < 1 || interfaceApChannel > 13) interfaceApChannel = WIFI_AP_CHANNEL;
  bool savedGenericSetup =
    strcmp(interfaceDeviceName, SETUP_DEVICE_NAME) == 0 &&
    strcmp(webUiUser, SETUP_AUTH_USER) == 0 &&
    strcmp(webUiPass, SETUP_AUTH_PASS) == 0;
  if (FORCE_INTERFACE_SETTINGS ||
      (savedGenericSetup &&
      (strcmp(DEVICE_NAME, SETUP_DEVICE_NAME) != 0 ||
       strcmp(WEBUI_AUTH_PASS, SETUP_AUTH_PASS) != 0 ||
       strcmp(INTERFACE_AP_PASS, SETUP_AUTH_PASS) != 0))) {
    strlcpy(interfaceDeviceName, DEVICE_NAME, sizeof(interfaceDeviceName));
    strlcpy(interfaceHostname, DEVICE_HOSTNAME, sizeof(interfaceHostname));
    strlcpy(interfaceApSsid, INTERFACE_AP_SSID, sizeof(interfaceApSsid));
    strlcpy(interfaceApPass, INTERFACE_AP_PASS, sizeof(interfaceApPass));
    strlcpy(webUiUser, WEBUI_AUTH_USER, sizeof(webUiUser));
    strlcpy(webUiPass, WEBUI_AUTH_PASS, sizeof(webUiPass));
    interfaceApChannel = WIFI_AP_CHANNEL;
    needsSeed = true;
  }
  if (needsSeed) {
    prefs.putString("devName", interfaceDeviceName);
    prefs.putString("hostname", interfaceHostname);
    prefs.putString("apSsid", interfaceApSsid);
    prefs.putString("apPass", interfaceApPass);
    prefs.putString("webUser", webUiUser);
    prefs.putString("webPass", webUiPass);
    prefs.putUChar("apChannel", interfaceApChannel);
  }
}

static bool usingDefaultWebCredentials() {
  return strcmp(webUiUser, SETUP_AUTH_USER) == 0 && strcmp(webUiPass, SETUP_AUTH_PASS) == 0;
}

static bool setupRouteAllowed() {
  String uri = server.uri();
  return uri == "/" || uri == "/status" || uri == "/interface";
}

static void saveInterfaceSettings(const String& deviceName, const String& hostname, const String& apSsid, uint8_t apChannel, const String& user, const String& pass) {
  String cleanDeviceName = deviceName;
  String cleanHostname = hostname;
  String cleanApSsid = apSsid;
  cleanDeviceName.trim();
  cleanHostname.trim();
  cleanApSsid.trim();
  if (cleanDeviceName.length()) {
    strlcpy(interfaceDeviceName, cleanDeviceName.c_str(), sizeof(interfaceDeviceName));
    prefs.putString("devName", interfaceDeviceName);
  }
  if (!cleanHostname.length()) cleanHostname = interfaceDeviceName;
  strlcpy(interfaceHostname, cleanHostname.c_str(), sizeof(interfaceHostname));
  prefs.putString("hostname", interfaceHostname);
  if (!cleanApSsid.length()) cleanApSsid = interfaceDeviceName;
  strlcpy(interfaceApSsid, cleanApSsid.c_str(), sizeof(interfaceApSsid));
  prefs.putString("apSsid", interfaceApSsid);
  interfaceApChannel = constrain(apChannel, (uint8_t)1, (uint8_t)13);
  prefs.putUChar("apChannel", interfaceApChannel);
  if (user.length()) {
    strlcpy(webUiUser, user.c_str(), sizeof(webUiUser));
    prefs.putString("webUser", webUiUser);
  }
  if (pass.length()) {
    strlcpy(interfaceApPass, pass.c_str(), sizeof(interfaceApPass));
    prefs.putString("apPass", interfaceApPass);
    strlcpy(webUiPass, pass.c_str(), sizeof(webUiPass));
    prefs.putString("webPass", webUiPass);
  }
}

static void startWifiAp() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPsetHostname(interfaceHostname);
  bool ok = WiFi.softAP(interfaceApSsid, interfaceApPass, interfaceApChannel);
  if (ok) {
    server.begin();
    wifiEnabled = true;
    wifiStartedMs = millis();
    appendLine(eventLog, LOG_SIZE, "[wifi] AP enabled\n");
  } else {
    wifiEnabled = false;
    appendLine(eventLog, LOG_SIZE, "[wifi] AP start failed\n");
  }
}

static void startWifiLocal() {
  if (!wifiLocalSsid[0]) {
    wifiEnabled = false;
    appendLine(eventLog, LOG_SIZE, "[wifi] Local SSID not set\n");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(interfaceHostname);
  WiFi.begin(wifiLocalSsid, wifiLocalPass);
  server.begin();
  wifiEnabled = true;
  wifiStartedMs = millis();
  appendLine(eventLog, LOG_SIZE, "[wifi] Local WiFi connecting\n");
}

static void startWifi() {
  if (wifiApMode) startWifiAp();
  else startWifiLocal();
}

static void stopWifi() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiEnabled = false;
  wifiStoppedMs = millis();
  wifiScanActive = false;
  wifiScanRequested = false;
  appendLine(eventLog, LOG_SIZE, "[wifi] radio disabled\n");
}

static void setWifiEnabled(bool enabled) {
  if (enabled == wifiEnabled) return;
  wifiToggleCount++;
  if (enabled) startWifi();
  else stopWifi();
}

static void setWifiApMode(bool apMode) {
  if (wifiApMode == apMode) return;
  bool wasEnabled = wifiEnabled;
  if (wasEnabled) stopWifi();
  wifiApMode = apMode;
  prefs.putBool("wifiApMode", wifiApMode);
  appendLine(eventLog, LOG_SIZE, wifiApMode ? "[wifi] mode set to AP\n" : "[wifi] mode set to Local\n");
  if (wasEnabled) startWifi();
}

static void saveWifiCredentials() {
  if (taWifiSsid) strlcpy(wifiLocalSsid, lv_textarea_get_text(taWifiSsid), sizeof(wifiLocalSsid));
  if (taWifiPass) strlcpy(wifiLocalPass, lv_textarea_get_text(taWifiPass), sizeof(wifiLocalPass));
  prefs.putString("wifiSsid", wifiLocalSsid);
  prefs.putString("wifiPass", wifiLocalPass);
  appendLine(eventLog, LOG_SIZE, "[wifi] Local credentials saved\n");
  if (wifiEnabled && !wifiApMode) {
    stopWifi();
    startWifi();
  }
}

static void renderWifiScanResults(int16_t status) {
  if (!listWifiScan) return;
  lv_obj_clean(listWifiScan);
  wifiScanResultCount = 0;
  if (status == WIFI_SCAN_RUNNING) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scanning...");
    return;
  }
  if (status == WIFI_SCAN_FAILED || status <= 0) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, status == 0 ? "No networks found" : "Scan failed");
    return;
  }

  wifiScanResultCount = min((size_t)status, WIFI_SCAN_MAX_RESULTS);
  if (lblWifiScanStatus) {
    char statusText[48];
    snprintf(statusText, sizeof(statusText), "%u network%s", (unsigned)wifiScanResultCount, wifiScanResultCount == 1 ? "" : "s");
    lv_label_set_text(lblWifiScanStatus, statusText);
  }
  for (size_t i = 0; i < wifiScanResultCount; i++) {
    strlcpy(wifiScanSsids[i], WiFi.SSID(i).c_str(), sizeof(wifiScanSsids[i]));
    wifiScanRssi[i] = WiFi.RSSI(i);
    lv_obj_t* btn = lv_btn_create(listWifiScan);
    lv_obj_set_size(btn, lv_pct(100), 42);
    styleDarkObject(btn, COLOR_PANEL);
    styleDarkBorder(btn, 0x2F705F);
    lv_obj_t* label = lv_label_create(btn);
    char itemText[64];
    snprintf(itemText, sizeof(itemText), "%s  %ld dBm", wifiScanSsids[i], (long)wifiScanRssi[i]);
    lv_label_set_text(label, itemText);
    styleBoundedLabel(label, SCREEN_W - 36, COLOR_TEXT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
      size_t index = (size_t)lv_event_get_user_data(e);
      if (index >= wifiScanResultCount) return;
      ensureWifiLocalPage();
      if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiScanSsids[index]);
      setWifiApMode(false);
      showPage(pageWifiLocal);
      if (taWifiPass) {
        lv_textarea_set_text(taWifiPass, "");
        lv_event_send(taWifiPass, LV_EVENT_FOCUSED, nullptr);
      }
    }, LV_EVENT_CLICKED, (void*)i);
  }
  WiFi.scanDelete();
}

static void renderIdfWifiScanResults(uint16_t count) {
  if (!listWifiScan) return;
  lv_obj_clean(listWifiScan);
  wifiScanResultCount = min((size_t)count, WIFI_SCAN_MAX_RESULTS);
  if (wifiScanResultCount == 0) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "No networks found");
    return;
  }

  wifi_ap_record_t records[WIFI_SCAN_MAX_RESULTS] = {};
  uint16_t readCount = (uint16_t)wifiScanResultCount;
  esp_err_t err = esp_wifi_scan_get_ap_records(&readCount, records);
  if (err != ESP_OK) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scan read failed");
    Serial.printf("[wifi] scan read failed err=%d\n", (int)err);
    return;
  }

  wifiScanResultCount = readCount;
  if (lblWifiScanStatus) {
    char statusText[48];
    snprintf(statusText, sizeof(statusText), "%u network%s", (unsigned)wifiScanResultCount, wifiScanResultCount == 1 ? "" : "s");
    lv_label_set_text(lblWifiScanStatus, statusText);
  }

  for (size_t i = 0; i < wifiScanResultCount; i++) {
    strlcpy(wifiScanSsids[i], (const char*)records[i].ssid, sizeof(wifiScanSsids[i]));
    wifiScanRssi[i] = records[i].rssi;
    lv_obj_t* btn = lv_btn_create(listWifiScan);
    lv_obj_set_size(btn, lv_pct(100), 42);
    styleDarkObject(btn, COLOR_PANEL);
    styleDarkBorder(btn, 0x2F705F);
    lv_obj_t* label = lv_label_create(btn);
    char itemText[64];
    snprintf(itemText, sizeof(itemText), "%s  %ld dBm", wifiScanSsids[i], (long)wifiScanRssi[i]);
    lv_label_set_text(label, itemText);
    styleBoundedLabel(label, SCREEN_W - 36, COLOR_TEXT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
      size_t index = (size_t)lv_event_get_user_data(e);
      if (index >= wifiScanResultCount) return;
      ensureWifiLocalPage();
      if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiScanSsids[index]);
      setWifiApMode(false);
      showPage(pageWifiLocal);
      if (taWifiPass) {
        lv_textarea_set_text(taWifiPass, "");
        lv_event_send(taWifiPass, LV_EVENT_FOCUSED, nullptr);
      }
    }, LV_EVENT_CLICKED, (void*)i);
  }
}

static void renderStoredWifiScanResults(int16_t status) {
  if (!listWifiScan) return;
  lv_obj_clean(listWifiScan);
  if (status == WIFI_SCAN_FAILED || status <= 0 || wifiScanResultCount == 0) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, status == 0 ? "No networks found" : "Scan failed");
    return;
  }

  if (lblWifiScanStatus) {
    char statusText[48];
    snprintf(statusText, sizeof(statusText), "%u network%s", (unsigned)wifiScanResultCount, wifiScanResultCount == 1 ? "" : "s");
    lv_label_set_text(lblWifiScanStatus, statusText);
  }

  for (size_t i = 0; i < wifiScanResultCount; i++) {
    lv_obj_t* btn = lv_btn_create(listWifiScan);
    lv_obj_set_size(btn, lv_pct(100), 42);
    styleDarkObject(btn, COLOR_PANEL);
    styleDarkBorder(btn, 0x2F705F);
    lv_obj_t* label = lv_label_create(btn);
    char itemText[64];
    snprintf(itemText, sizeof(itemText), "%s  %ld dBm", wifiScanSsids[i], (long)wifiScanRssi[i]);
    lv_label_set_text(label, itemText);
    styleBoundedLabel(label, SCREEN_W - 36, COLOR_TEXT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
      size_t index = (size_t)lv_event_get_user_data(e);
      if (index >= wifiScanResultCount) return;
      ensureWifiLocalPage();
      if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiScanSsids[index]);
      setWifiApMode(false);
      showPage(pageWifiLocal);
      if (taWifiPass) {
        lv_textarea_set_text(taWifiPass, "");
        lv_event_send(taWifiPass, LV_EVENT_FOCUSED, nullptr);
      }
    }, LV_EVENT_CLICKED, (void*)i);
  }
}

static void wifiScanTask(void*) {
  int16_t status = WIFI_SCAN_FAILED;
  size_t resultCount = 0;
  Serial.println("[wifi] task scan starting");
  WiFi.mode(WIFI_STA);
  WiFi.scanDelete();
  status = WiFi.scanNetworks(false, false, false, 180);
  if (status > 0) {
    resultCount = min((size_t)status, WIFI_SCAN_MAX_RESULTS);
    for (size_t i = 0; i < resultCount; i++) {
      strlcpy(wifiScanSsids[i], WiFi.SSID(i).c_str(), sizeof(wifiScanSsids[i]));
      wifiScanRssi[i] = WiFi.RSSI(i);
    }
  }
  WiFi.scanDelete();
  wifiScanResultCount = resultCount;
  wifiScanTaskStatus = status;
  wifiScanTaskDone = true;
  wifiScanTaskRunning = false;
  Serial.printf("[wifi] task scan complete status=%d count=%u\n", status, (unsigned)resultCount);
  vTaskDelete(nullptr);
}

static void requestWifiScan() {
  wifiScanRequested = true;
  wifiScanActive = false;
  if (listWifiScan) lv_obj_clean(listWifiScan);
  if (lblWifiScanStatus) {
    lv_label_set_text(lblWifiScanStatus, "Scan requested...");
  }
}

static void startWifiScan() {
  if (wifiScanActive || !wifiScanRequested) return;
  wifiScanRequested = false;
  wifiScanActive = true;
  renderWifiScanResults(WIFI_SCAN_RUNNING);
  wifiScanStartedMs = millis();
  wifiScanTaskDone = false;
  wifiScanTaskStatus = WIFI_SCAN_RUNNING;
  wifiScanResultCount = 0;
  if (wifiScanTaskRunning) {
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Previous scan still running");
    wifiScanActive = false;
    return;
  }
  wifiScanTaskRunning = true;
  BaseType_t ok = xTaskCreatePinnedToCore(wifiScanTask, "wifiScan", 8192, nullptr, 1, nullptr, 0);
  Serial.printf("[wifi] scan task create=%ld\n", (long)ok);
  if (ok != pdPASS) {
    wifiScanTaskRunning = false;
    wifiScanActive = false;
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scan task failed");
  }
}

static void pollWifiScan() {
  if (wifiScanRequested && wifiEnabled && !wifiScanStoppedWifi) {
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiEnabled = false;
    wifiStoppedMs = millis();
    wifiScanStoppedWifi = true;
    wifiScanRequestedMs = millis();
    appendLine(eventLog, LOG_SIZE, "[wifi] disabled for scan\n");
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Starting scan...");
    return;
  }
  if (wifiScanRequested && millis() - wifiScanRequestedMs >= 750) {
    startWifiScan();
  }
  if (!wifiScanActive) return;
  if (wifiScanTaskDone) {
    wifiScanActive = false;
    wifiScanTaskDone = false;
    renderStoredWifiScanResults(wifiScanTaskStatus);
    return;
  }
  if (millis() - wifiScanStartedMs > 9000) {
    wifiScanActive = false;
    Serial.println("[wifi] task scan timeout");
    if (lblWifiScanStatus) lv_label_set_text(lblWifiScanStatus, "Scan timed out");
  }
}

static void applyBacklight() {
  uint8_t pct = constrain(backlightPercent, 10, 100);
  uint32_t duty = map(pct, 0, 100, 0, 255);
  if (backlightPwmReady) {
    ledcWrite(TFT_BL, duty);
  } else {
    analogWrite(TFT_BL, duty);
  }
}

static void initBacklight() {
  Serial.printf("[boot] Backlight pin %d configuring...\n", TFT_BL);
  pinMode(TFT_BL, OUTPUT);
  backlightPwmReady = ledcAttach(TFT_BL, 5000, 8);
  Serial.printf("[boot] Backlight PWM ready: %d\n", backlightPwmReady);
  applyBacklight();
}

static void applyFontMode() {
  if (readableTextMode) {
    UI_FONT_SMALL = LV_FONT_DEFAULT;
    UI_FONT_DEFAULT = UI_FONT_READABLE;
  } else {
    UI_FONT_SMALL = &comic_neue_12;
    UI_FONT_DEFAULT = &comic_neue_14;
  }
}

static void backlightSliderEvent(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  backlightPercent = constrain((int)lv_slider_get_value(slider), 10, 100);
  applyBacklight();
}

static void lvFlush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* colors) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)colors, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

static bool readTouch(uint16_t& x, uint16_t& y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)5) != 5) return false;

  uint8_t touches = Wire.read();
  uint8_t xh = Wire.read();
  uint8_t xl = Wire.read();
  uint8_t yh = Wire.read();
  uint8_t yl = Wire.read();
  if (touches == 0 || touches > 2) return false;

  uint16_t rawX = constrain(((uint16_t)(xh & 0x0F) << 8) | xl, 0, SCREEN_W - 1);
  uint16_t rawY = constrain(((uint16_t)(yh & 0x0F) << 8) | yl, 0, SCREEN_H - 1);
  if (landscapeKeyboardOpen) {
    x = rawY;
    y = SCREEN_W - 1 - rawX;
  } else {
    x = rawX;
    y = rawY;
  }
  return true;
}

static void lvTouchRead(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  uint16_t x = 0;
  uint16_t y = 0;
  if (readTouch(x, y)) {
    touchSamples++;
    lastTouchMs = millis();
    lastTouchX = x;
    lastTouchY = y;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void styleDarkObject(lv_obj_t* obj, uint32_t bg, uint32_t text) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_color(obj, lv_color_hex(text), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

static void styleDarkBorder(lv_obj_t* obj, uint32_t color) {
  lv_obj_set_style_border_color(obj, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
}

static void styleDarkTextArea(lv_obj_t* ta) {
  styleDarkObject(ta, COLOR_INPUT, 0xE8FFF5);
  styleDarkBorder(ta, 0x2F705F);
  lv_obj_set_style_text_font(ta, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_line_space(ta, 1, 0);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_MUTED), LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0x16342C), LV_PART_SELECTED);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_TEXT), LV_PART_SELECTED);
}

static void styleBoundedLabel(lv_obj_t* label, lv_coord_t width, uint32_t color, lv_label_long_mode_t mode) {
  if (!label) return;
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, mode);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
}

static lv_obj_t* makePanel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  styleDarkObject(panel, COLOR_PANEL);
  styleDarkBorder(panel);
  lv_obj_set_style_radius(panel, 6, 0);
  lv_obj_set_style_pad_all(panel, 5, 0);
  return panel;
}

static lv_obj_t* makePage(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H - NAV_BAR_H - STATUS_BAR_H);
  lv_obj_align(page, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_H);
  styleDarkObject(page, COLOR_BG);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 5, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  return page;
}

static void makePageScrollable(lv_obj_t* page) {
  lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
}

static void buildStatusBar(lv_obj_t* screen) {
  statusBar = lv_obj_create(screen);
  lv_obj_set_size(statusBar, SCREEN_W, STATUS_BAR_H);
  lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);
  styleDarkObject(statusBar, 0x080D0B, COLOR_MUTED);
  lv_obj_set_style_border_color(statusBar, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_side(statusBar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(statusBar, 1, 0);
  lv_obj_set_style_pad_hor(statusBar, 6, 0);
  lv_obj_set_style_pad_ver(statusBar, 2, 0);
  lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);

  lblStatus = lv_label_create(statusBar);
  lv_label_set_text(lblStatus, "booting");
  lv_obj_set_width(lblStatus, SCREEN_W - 158);
  lv_label_set_long_mode(lblStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(lblStatus, UI_FONT_SMALL, 0);
  lv_obj_align(lblStatus, LV_ALIGN_LEFT_MID, 0, 0);

  lblRadioStatus = lv_label_create(statusBar);
  lv_label_set_text(lblRadioStatus, LV_SYMBOL_WIFI);
  lv_obj_set_width(lblRadioStatus, 28);
  lv_label_set_long_mode(lblRadioStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblRadioStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lblRadioStatus, lv_color_hex(0xFF5A5F), 0);
  lv_obj_set_style_text_font(lblRadioStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblRadioStatus, LV_ALIGN_RIGHT_MID, -126, 0);

  lblGpsStatus = lv_label_create(statusBar);
  lv_label_set_text(lblGpsStatus, LV_SYMBOL_GPS " 0");
  lv_obj_set_width(lblGpsStatus, 40);
  lv_label_set_long_mode(lblGpsStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblGpsStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lblGpsStatus, lv_color_hex(0xFF5A5F), 0);
  lv_obj_set_style_text_font(lblGpsStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblGpsStatus, LV_ALIGN_RIGHT_MID, -84, 0);

  lblBatteryStatus = lv_label_create(statusBar);
  lv_label_set_text(lblBatteryStatus, "Batt --%");
  lv_obj_set_width(lblBatteryStatus, 82);
  lv_label_set_long_mode(lblBatteryStatus, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(lblBatteryStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_color(lblBatteryStatus, lv_color_hex(COLOR_ACCENT), 0);
  // LVGL symbols require the built-in symbol-capable font.
  lv_obj_set_style_text_font(lblBatteryStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblBatteryStatus, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void showPage(lv_obj_t* target, bool remember) {
  if (!target) return;
  bool leavingNodeDetailMap = currentPage == pageGps && target != pageGps && mapNodeDetailViewActive;
  if (remember && currentPage && currentPage != target) previousPage = currentPage;
  lv_obj_t* pages[] = {
    pageLauncher, pageOperate, pageDiagnose, pageNodes, pageNodeDetail, pageMeshHealth, pagePacketInspector,
    pageTxHistory, pagePublicChat, pagePrivateChat, pageDirectChat, pageGps,
    pageSystem, pageSystemInterface, pageSystemSerial, pageSystemRadio, pageSystemGps,
    pageWifi, pageWifiStats, pageWifiLocal, pageWifiScan, pageBacklight, pageBattery
  };
  for (lv_obj_t* page : pages) {
    if (!page) continue;
    if (page == target) lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  currentPage = target;
  if (target == pagePublicChat) unreadPublic = 0;
  if (target == pagePrivateChat) unreadFamily = 0;
  if (target == pageDirectChat) unreadDirect = 0;
  if (leavingNodeDetailMap) {
    mapNodeDetailViewActive = false;
    mapFocusSelectedNode = false;
    mapNearbyMode = false;
    mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
    mapCanvasCached = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
  }
  forcePageRefresh = true;
  refreshDashboardLabels();
  if (target == pageGps) {
    if (!mapNodeDetailViewActive) {
      mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
      mapFocusSelectedNode = false;
    }
    if (!mapFocusSelectedNode) mapNearbyMode = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
    if (mapCanvasCached && lblMapStats) {
      char cacheText[180];
      if (lastMapLocationValid) {
        snprintf(cacheText, sizeof(cacheText),
                 "Current location ready\n"
                 "Default: %.5f, %.5f\n"
                 "Checking SD tiles...",
                 defaultMapLat,
                 defaultMapLon);
      } else {
        snprintf(cacheText, sizeof(cacheText),
                 "Map ready\n"
                 "Waiting for current device location\n"
                 "Checking SD tiles...");
      }
      lv_label_set_text(lblMapStats, cacheText);
    }
  }
}

static void setMapNearbyMode(bool enabled) {
  mapNearbyMode = enabled;
  mapFocusSelectedNode = false;
  mapNodeDetailViewActive = false;
  mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
  mapRenderPending = true;
  lastMapUiRefreshMs = 0;
  refreshMapUi();
}

static int activeMapPlotHeight() {
  return mapExpanded ? MAP_PLOT_H_EXPANDED : MAP_PLOT_H_NORMAL;
}

static void setMapExpanded(bool expanded) {
  mapExpanded = expanded;
  mapCanvasCached = false;
  mapRenderPending = true;
  markUiDirty(UI_DIRTY_MAP);
  lastMapUiRefreshMs = 0;
  if (mapPlot) lv_obj_set_size(mapPlot, MAP_PLOT_W, activeMapPlotHeight());
  if (lblMapStats) {
    if (expanded) lv_obj_add_flag(lblMapStats, LV_OBJ_FLAG_HIDDEN);
    else {
      lv_obj_clear_flag(lblMapStats, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(lblMapStats, LV_ALIGN_TOP_LEFT, 6, 164);
    }
  }
  refreshMapUi();
}

static void setMapZoomedOut(bool zoomedOut) {
  mapZoomedOut = zoomedOut;
  mapCanvasCached = false;
  mapRenderPending = true;
  markUiDirty(UI_DIRTY_MAP);
  lastMapUiRefreshMs = 0;
  refreshMapUi();
}

static void clearUiObjectPointers() {
  pageLauncher = nullptr;
  pageOperate = nullptr;
  pageDiagnose = nullptr;
  pageNodes = nullptr;
  pageNodeDetail = nullptr;
  pageMeshHealth = nullptr;
  pagePacketInspector = nullptr;
  pageTxHistory = nullptr;
  pagePublicChat = nullptr;
  pagePrivateChat = nullptr;
  pageDirectChat = nullptr;
  pageGps = nullptr;
  pageSystem = nullptr;
  pageSystemInterface = nullptr;
  pageSystemSerial = nullptr;
  pageSystemRadio = nullptr;
  pageSystemGps = nullptr;
  pageWifi = nullptr;
  pageWifiStats = nullptr;
  pageWifiLocal = nullptr;
  pageWifiScan = nullptr;
  pageBacklight = nullptr;
  pageBattery = nullptr;
  currentPage = nullptr;
  previousPage = nullptr;
  statusBar = nullptr;
  navBar = nullptr;
  lblStatus = nullptr;
  lblRadioStatus = nullptr;
  lblGpsStatus = nullptr;
  lblBatteryStatus = nullptr;
  lblStats = nullptr;
  lblHomeLink = nullptr;
  lblHomeMessages = nullptr;
  lblHomeGps = nullptr;
  lblHomePower = nullptr;
  lblTilePublic = nullptr;
  lblTilePrivate = nullptr;
  lblMsgPublic = nullptr;
  lblMsgFamily = nullptr;
  lblMsgDirect = nullptr;
  lblPublicChatTitle = nullptr;
  lblPrivateChatTitle = nullptr;
  lblPublicChannelStats = nullptr;
  lblPrivateChannelStats = nullptr;
  lblPrivateChannelHint = nullptr;
  listNodes = nullptr;
  lblNodeListMode = nullptr;
  taNodeDetail = nullptr;
  nodeDetailLastText[0] = '\0';
  nodeDetailRenderedNode = 0;
  nodeDetailScrollTopPending = false;
  lblMeshHealth = nullptr;
  taPacketInspector = nullptr;
  taTxHistory = nullptr;
  lblSystemInterface = nullptr;
  lblSystemSerial = nullptr;
  lblSystemRadio = nullptr;
  lblWifiState = nullptr;
  lblWifiStats = nullptr;
  lblWifiScanStatus = nullptr;
  swWifiEnabled = nullptr;
  swWifiApMode = nullptr;
  taWifiSsid = nullptr;
  taWifiPass = nullptr;
  listWifiScan = nullptr;
  sliderBacklight = nullptr;
  lblBacklight = nullptr;
  swReadableText = nullptr;
  lblTextMode = nullptr;
  lblBatteryStats = nullptr;
  lblGpsStats = nullptr;
  lblMapStats = nullptr;
  mapPlot = nullptr;
  mapCanvas = nullptr;
  mapNodeDetailViewActive = false;
  mapFocusSelectedNode = false;
  mapNearbyMode = false;
  mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
  memset(mapDots, 0, sizeof(mapDots));
  taPublicChat = nullptr;
  taFamilyChat = nullptr;
  taDirectChat = nullptr;
  taScreenLog = nullptr;
  taScreenNodes = nullptr;
  taPublicInput = nullptr;
  taFamilyInput = nullptr;
  taDirectTo = nullptr;
  taDirectInput = nullptr;
  activeChatInput = nullptr;
  keyboard = nullptr;
  mainScreen = nullptr;
  wifiLocalPageBuilt = false;
  wifiScanPageBuilt = false;
}

static lv_obj_t* makeActionButton(lv_obj_t* parent, const char* text, int y, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, UI_ACTION_W, UI_ACTION_H);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, UI_ACTION_W - 20);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeSmallButton(lv_obj_t* parent, const char* text, int x, int y, int w, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, 22);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, w - 8);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeSystemTile(lv_obj_t* parent, const char* text, int col, int row, lv_event_cb_t cb) {
  const int tileW = UI_TILE_W;
  const int tileH = UI_TILE_H;
  const int gap = UI_GAP;
  const int x = 6 + col * (tileW + gap);
  const int y = 24 + row * (tileH + gap);
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, tileW, tileH);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, tileW - 12);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeDashboardLabel(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* panel = makePanel(parent);
  lv_obj_set_size(panel, w, h);
  lv_obj_align(panel, LV_ALIGN_TOP_LEFT, x, y);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* label = lv_label_create(panel);
  lv_obj_set_width(label, w - 10);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
  return label;
}

static lv_obj_t* makeDashboardAction(lv_obj_t* parent, const char* text, int col, int row, lv_event_cb_t cb) {
  const int w = 108;
  const int h = 34;
  const int x = 6 + col * 114;
  const int y = 158 + row * 38;
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  styleDarkObject(btn, col == 0 && row == 0 ? COLOR_ACTION : COLOR_PANEL, col == 0 && row == 0 ? 0x001B12 : COLOR_TEXT);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, w - 10);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(col == 0 && row == 0 ? 0x001B12 : COLOR_TEXT), 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makePageTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* title = lv_label_create(parent);
  lv_label_set_text(title, text);
  lv_obj_set_width(title, SCREEN_W - 12);
  lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_text_font(title, UI_FONT_SMALL, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 2, 0);
  return title;
}

static lv_obj_t* makeNavButton(lv_obj_t* parent, const char* text, lv_align_t align, int x, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 92, 30);
  lv_obj_align(btn, align, x, 0);
  styleDarkObject(btn, 0x111A18);
  styleDarkBorder(btn, 0x315B50);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, 80);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static void buildNavBar(lv_obj_t* screen) {
  navBar = lv_obj_create(screen);
  lv_obj_set_size(navBar, SCREEN_W, NAV_BAR_H);
  lv_obj_align(navBar, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(navBar, 0x080D0B);
  lv_obj_set_style_border_color(navBar, lv_color_hex(COLOR_BORDER), 0);
  lv_obj_set_style_border_side(navBar, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(navBar, 1, 0);
  lv_obj_set_style_pad_all(navBar, 4, 0);
  lv_obj_clear_flag(navBar, LV_OBJ_FLAG_SCROLLABLE);

  makeNavButton(navBar, "Back", LV_ALIGN_LEFT_MID, 4, [](lv_event_t*) {
    if (previousPage && currentPage != pageLauncher) {
      lv_obj_t* target = previousPage;
      previousPage = pageLauncher;
      showPage(target, false);
    } else {
      showPage(pageLauncher, false);
    }
  });
  makeNavButton(navBar, "Home", LV_ALIGN_RIGHT_MID, -4, [](lv_event_t*) {
    previousPage = currentPage;
    showPage(pageLauncher, false);
  });
}

static lv_obj_t* makeReadonlyText(lv_obj_t* parent, int y, int h) {
  lv_obj_t* ta = lv_textarea_create(parent);
  lv_obj_set_size(ta, SCREEN_W - 16, h);
  lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, y);
  lv_textarea_set_cursor_click_pos(ta, false);
  lv_obj_clear_flag(ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  styleDarkTextArea(ta);
  return ta;
}

static bool setReadonlyTextStable(lv_obj_t* ta, const char* text, bool scrollTop = false) {
  if (!ta || !text) return false;
  const char* current = lv_textarea_get_text(ta);
  bool same = current && strcmp(current, text) == 0;
  lv_coord_t scrollY = scrollTop ? 0 : lv_obj_get_scroll_y(ta);
  if (!same) {
    lv_textarea_set_text(ta, text);
  }
  if (!same || scrollTop) {
    lv_textarea_set_cursor_pos(ta, 0);
    lv_obj_update_layout(ta);
    lv_obj_scroll_to_y(ta, scrollY, LV_ANIM_OFF);
  }
  return !same;
}

static NodeRecord* findNode(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num) return &nodes[i];
  }
  return nullptr;
}

static uint32_t nodeAgeSeconds(const NodeRecord& node, uint32_t nowMs) {
  return node.lastHeardMs ? (nowMs - node.lastHeardMs) / 1000 : 0;
}

static double headingTo(double lat1, double lon1, double lat2, double lon2) {
  double dLon = (lon2 - lon1) * DEG_TO_RAD;
  double y = sin(dLon) * cos(lat2 * DEG_TO_RAD);
  double x = cos(lat1 * DEG_TO_RAD) * sin(lat2 * DEG_TO_RAD) -
             sin(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) * cos(dLon);
  double brng = atan2(y, x) * RAD_TO_DEG;
  return fmod(brng + 360.0, 360.0);
}

static const char* nodeSortLabel() {
  switch (nodeSortMode) {
    case NODE_SORT_SNR: return "snr";
    case NODE_SORT_DISTANCE: return "range";
    case NODE_SORT_NAME: return "name";
    default: return "heard";
  }
}

static const char* nodeFilterLabel() {
  switch (nodeFilterMode) {
    case NODE_FILTER_ACTIVE: return "active";
    case NODE_FILTER_POSITIONED: return "position";
    case NODE_FILTER_STALE: return "stale";
    case NODE_FILTER_NEARBY: return "nearby";
    default: return "all";
  }
}

static bool nodeHasRange(const NodeRecord& node) {
  return gpsStats.valid && node.hasPosition;
}

static double nodeRangeMeters(const NodeRecord& node) {
  if (!nodeHasRange(node)) return -1.0;
  return distanceMeters(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
}

static const char* nodeStatusLabel(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return "STALE";
  if (node.snr < -10.0f) return "WEAK";
  return "ACTIVE";
}

static uint32_t nodeRowBg(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return 0x0B100F;
  if (node.snr < -10.0f) return 0x1D1810;
  return 0x10201B;
}

static uint32_t nodeRowBorder(const NodeRecord& node, uint32_t nowMs) {
  if (nodeAgeSeconds(node, nowMs) > 900) return 0x2A3A35;
  if (node.snr < -10.0f) return 0x6F5A2E;
  return 0x2F705F;
}

static bool nodeMatchesFilter(const NodeRecord& node, uint32_t nowMs) {
  uint32_t age = nodeAgeSeconds(node, nowMs);
  switch (nodeFilterMode) {
    case NODE_FILTER_ACTIVE: return age <= 900;
    case NODE_FILTER_POSITIONED: return node.hasPosition;
    case NODE_FILTER_STALE: return age > 900;
    case NODE_FILTER_NEARBY: return nodeHasRange(node) && nodeRangeMeters(node) <= NODE_NEARBY_METERS;
    default: return true;
  }
}

static bool nodeComesBefore(const NodeRecord& a, const NodeRecord& b, uint32_t nowMs) {
  switch (nodeSortMode) {
    case NODE_SORT_SNR:
      return a.snr > b.snr;
    case NODE_SORT_DISTANCE:
      if (gpsStats.valid) {
        bool aHasRange = a.hasPosition;
        bool bHasRange = b.hasPosition;
        if (aHasRange && bHasRange) {
          double da = distanceMeters(gpsStats.latitude, gpsStats.longitude, a.latitude, a.longitude);
          double db = distanceMeters(gpsStats.latitude, gpsStats.longitude, b.latitude, b.longitude);
          return da < db;
        }
        if (aHasRange != bHasRange) return aHasRange;
      }
      return nodeAgeSeconds(a, nowMs) < nodeAgeSeconds(b, nowMs);
    case NODE_SORT_NAME:
      return strcasecmp(a.name, b.name) < 0;
    default:
      return nodeAgeSeconds(a, nowMs) < nodeAgeSeconds(b, nowMs);
  }
}

static void formatRangeBearing(const NodeRecord& node, char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return;
  if (!gpsStats.valid || !node.hasPosition) {
    strlcpy(buffer, "range --", bufferSize);
    return;
  }
  double meters = distanceMeters(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
  double bearing = headingTo(gpsStats.latitude, gpsStats.longitude, node.latitude, node.longitude);
  if (meters >= 1000.0) {
    snprintf(buffer, bufferSize, "%.1f km %.0f deg", meters / 1000.0, bearing);
  } else {
    snprintf(buffer, bufferSize, "%.0f m %.0f deg", meters, bearing);
  }
}

static void nodeListButtonEvent(lv_event_t* e) {
  uint32_t nodeNum = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
  showNodeDetail(nodeNum);
}

static void cycleNodeSort() {
  nodeSortMode = (NodeSortMode)((nodeSortMode + 1) % 4);
  refreshNodeList(true);
}

static void cycleNodeFilter() {
  nodeFilterMode = (NodeFilterMode)((nodeFilterMode + 1) % 5);
  refreshNodeList(true);
}

static void clearNodeList() {
  memset(nodes, 0, sizeof(nodes));
  nodeCount = 0;
  selectedNodeNum = 0;
  nodeSortMode = NODE_SORT_LAST_HEARD;
  nodeFilterMode = NODE_FILTER_ALL;
  lastNodeListRefreshMs = 0;
  nodeListClearedMs = millis();
  stats.onlineNodes = 0;
  stats.totalNodes = 0;
  mapRenderPending = true;
  appendLine(eventLog, LOG_SIZE, "[local] node list cleared\n");
  sendHeltecNodeDbReset();
  refreshNodeList(true);
  refreshDashboardLabels();
}

static void refreshNodeList(bool force) {
  if (!listNodes) return;
  uint32_t now = millis();
  if (!force && now - lastNodeListRefreshMs < 3000) return;
  lastNodeListRefreshMs = now;
  lv_obj_clean(listNodes);
  if (lblNodeListMode) {
    char modeText[48];
    snprintf(modeText, sizeof(modeText), "Sort: %s  Filter: %s", nodeSortLabel(), nodeFilterLabel());
    lv_label_set_text(lblNodeListMode, modeText);
  }
  size_t sorted[MAX_NODES];
  size_t sortedCount = 0;
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodeMatchesFilter(nodes[i], now)) sorted[sortedCount++] = i;
  }
  for (size_t i = 0; i < sortedCount; i++) {
    for (size_t j = i + 1; j < sortedCount; j++) {
      if (nodeComesBefore(nodes[sorted[j]], nodes[sorted[i]], now)) {
        size_t tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }
  if (sortedCount == 0) {
    const char* emptyText = nodeCount ? "No nodes match filter" : "No nodes heard yet";
    if (!nodeCount && nodeListClearedMs) emptyText = "Node list reset; waiting for fresh packets";
    lv_list_add_text(listNodes, emptyText);
    clearUiDirty(UI_DIRTY_NODES);
    return;
  }
  for (size_t listIndex = 0; listIndex < sortedCount; listIndex++) {
    NodeRecord& node = nodes[sorted[listIndex]];
    char title[48];
    snprintf(title, sizeof(title), "%.30s", node.name[0] ? node.name : nodeName(node.num));
    uint32_t ageSeconds = nodeAgeSeconds(node, now);

    lv_obj_t* row = lv_obj_create(listNodes);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 60);
    styleDarkObject(row, nodeRowBg(node, now));
    styleDarkBorder(row, nodeRowBorder(node, now));
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, nodeListButtonEvent, LV_EVENT_CLICKED, (void*)(uintptr_t)node.num);

    lv_obj_t* nameLabel = lv_label_create(row);
    lv_label_set_text(nameLabel, title);
    lv_obj_set_width(nameLabel, 126);
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(nameLabel, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(nameLabel, UI_FONT_SMALL, 0);
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    char badgeText[24];
    snprintf(badgeText, sizeof(badgeText), "%s %s", nodeStatusLabel(node, now), node.hasPosition ? "GPS" : "NO GPS");
    lv_obj_t* badgeLabel = lv_label_create(row);
    lv_label_set_text(badgeLabel, badgeText);
    lv_obj_set_width(badgeLabel, 78);
    lv_label_set_long_mode(badgeLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(badgeLabel, lv_color_hex(node.hasPosition ? COLOR_ACCENT : COLOR_MUTED), 0);
    lv_obj_set_style_text_align(badgeLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(badgeLabel, UI_FONT_SMALL, 0);
    lv_obj_align(badgeLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

    char rangeText[32];
    formatRangeBearing(node, rangeText, sizeof(rangeText));
    char detail[128];
    snprintf(detail, sizeof(detail), "!%08lX  H%u  %lus\n%.1f dB  %ld dBm  %s",
             (unsigned long)node.num,
             node.hopsAway,
             (unsigned long)ageSeconds,
             node.snr,
             (long)node.rssi,
             rangeText);
    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, detail);
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_MUTED), 0);
    lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 16);
  }
  clearUiDirty(UI_DIRTY_NODES);
}

static void directSelectedNode() {
  if (!selectedNodeNum || !taDirectTo) return;
  char toText[12];
  snprintf(toText, sizeof(toText), "!%08lX", (unsigned long)selectedNodeNum);
  lv_textarea_set_text(taDirectTo, toText);
  showPage(pageDirectChat);
}

static void showSelectedNodeOnMap() {
  if (!selectedNodeNum) return;
  NodeRecord* selected = findNode(selectedNodeNum);
  mapNearbyMode = false;
  mapNodeDetailViewActive = selected != nullptr;
  mapCenterMode = selected ? MAP_CENTER_SELECTED_NODE : MAP_CENTER_CURRENT_DEVICE;
  mapFocusSelectedNode = selected && selected->hasPosition;
  mapCanvasCached = false;
  mapRenderPending = true;
  markUiDirty(UI_DIRTY_MAP);
  lastMapUiRefreshMs = 0;
  showPage(pageGps);
}

static void showNodeDetail(uint32_t nodeNum) {
  if (selectedNodeNum != nodeNum) {
    nodeDetailLastText[0] = '\0';
  }
  selectedNodeNum = nodeNum;
  nodeDetailScrollTopPending = true;
  showPage(pageNodeDetail);
}

static int8_t activeChatChannel() {
  if (activeChatInput == taFamilyInput) {
    return privateChannelIndex >= 0 ? privateChannelIndex : 1;
  }
  return PUBLIC_CHANNEL_INDEX;
}

static void sendFromInput(lv_obj_t* input, int8_t channelIndex) {
  if (!input) return;
  const char* msg = lv_textarea_get_text(input);
  if (msg && msg[0] && sendTextMessage(msg, channelIndex)) {
    lv_textarea_set_text(input, "");
  }
}

static void sendActiveFromScreen() {
  if (activeChatInput == taDirectInput) {
    sendDirectFromInput(taDirectTo, taDirectInput);
  } else if (activeChatInput != taDirectTo) {
    sendFromInput(activeChatInput, activeChatChannel());
  }
}

static void setUiLandscape(bool landscape) {
  if (!display) return;
  tft.setRotation(landscape ? 1 : 0);
  tft.invertDisplay(true);
  dispDrv.hor_res = landscape ? SCREEN_H : SCREEN_W;
  dispDrv.ver_res = landscape ? SCREEN_W : SCREEN_H;
  lv_disp_drv_update(display, &dispDrv);
  tft.fillScreen(TFT_BLACK);
}

static void closeLandscapeKeyboard(bool send) {
  if (!landscapeKeyboardOpen) return;
  if (keyboardText && activeChatInput) {
    lv_textarea_set_text(activeChatInput, lv_textarea_get_text(keyboardText));
  }
  if (send && landscapeKeyboardSendsMessage) {
    sendActiveFromScreen();
  }
  if (landscapeKeyboard) lv_keyboard_set_textarea(landscapeKeyboard, nullptr);
  landscapeKeyboardOpen = false;
  setUiLandscape(false);
  if (mainScreen) lv_scr_load(mainScreen);
}

static void landscapeKeyboardEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    closeLandscapeKeyboard(true);
  } else if (code == LV_EVENT_CANCEL) {
    closeLandscapeKeyboard(false);
  }
}

static void openLandscapeKeyboard(lv_obj_t* input, const char* prompt, size_t maxLength, bool sendsMessage) {
  if (!input || !keyboardScreen || !keyboardPrompt || !keyboardText || !landscapeKeyboard) return;
  activeChatInput = input;
  landscapeKeyboardSendsMessage = sendsMessage;
  lv_label_set_text(keyboardPrompt, prompt ? prompt : "Text");
  lv_textarea_set_text(keyboardText, lv_textarea_get_text(input));
  lv_textarea_set_max_length(keyboardText, maxLength);
  lv_keyboard_set_textarea(landscapeKeyboard, keyboardText);
  landscapeKeyboardOpen = true;
  lv_obj_clear_state(input, LV_STATE_FOCUSED);
  setUiLandscape(true);
  lv_scr_load(keyboardScreen);
  lv_obj_add_state(keyboardText, LV_STATE_FOCUSED);
}

static void inputEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_FOCUSED) {
    lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
    if (target == taDirectTo) openLandscapeKeyboard(target, "Direct recipient", 9, false);
    else if (target == taDirectInput) openLandscapeKeyboard(target, "Direct message", 233, true);
    else if (target == taFamilyInput) {
      char prompt[40];
      formatChannelPrompt(privateChannelLabel(), prompt, sizeof(prompt));
      openLandscapeKeyboard(target, prompt, 233, true);
    } else {
      char prompt[40];
      formatChannelPrompt(publicChannelLabel(), prompt, sizeof(prompt));
      openLandscapeKeyboard(target, prompt, 233, true);
    }
  } else if (code == LV_EVENT_DEFOCUSED) {
    if (!landscapeKeyboardOpen && keyboard) {
      lv_keyboard_set_textarea(keyboard, nullptr);
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  } else if (code == LV_EVENT_READY) {
    sendActiveFromScreen();
    if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  } else if (code == LV_EVENT_CANCEL) {
    if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  }
}

static void wifiInputEvent(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_FOCUSED) return;
  lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
  openLandscapeKeyboard(target, target == taWifiPass ? "WiFi password" : "WiFi SSID", target == taWifiPass ? 64 : 32, false);
}

static void deferWifiAction(uint8_t action) {
  deferredWifiAction = action;
  deferredWifiActionMs = millis();
}

static void ensureWifiLocalPage() {
  if (wifiLocalPageBuilt || !pageWifiLocal) return;
  wifiLocalPageBuilt = true;

  makePageTitle(pageWifiLocal, "Local Network");
  lv_obj_t* localWifiPanel = makePanel(pageWifiLocal);
  lv_obj_set_size(localWifiPanel, SCREEN_W - 12, 220);
  lv_obj_align(localWifiPanel, LV_ALIGN_TOP_MID, 0, 24);

  lv_obj_t* btnScanWifi = lv_btn_create(localWifiPanel);
  lv_obj_set_size(btnScanWifi, SCREEN_W - 40, 40);
  lv_obj_align(btnScanWifi, LV_ALIGN_TOP_MID, 0, 8);
  styleDarkObject(btnScanWifi, 0x2F705F, 0xFFFFFF);
  lv_obj_t* lblScanWifi = lv_label_create(btnScanWifi);
  lv_label_set_text(lblScanWifi, "Scan WiFi");
  styleBoundedLabel(lblScanWifi, SCREEN_W - 56, 0xFFFFFF);
  lv_obj_center(lblScanWifi);
  lv_obj_add_event_cb(btnScanWifi, [](lv_event_t*) {
    deferWifiAction(2);
  }, LV_EVENT_CLICKED, nullptr);

  taWifiSsid = lv_textarea_create(localWifiPanel);
  lv_obj_set_size(taWifiSsid, SCREEN_W - 40, 36);
  lv_obj_align(taWifiSsid, LV_ALIGN_TOP_MID, 0, 56);
  styleDarkTextArea(taWifiSsid);
  lv_textarea_set_one_line(taWifiSsid, true);
  lv_textarea_set_max_length(taWifiSsid, 32);
  lv_textarea_set_placeholder_text(taWifiSsid, "SSID");
  lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  lv_obj_add_event_cb(taWifiSsid, wifiInputEvent, LV_EVENT_FOCUSED, nullptr);

  taWifiPass = lv_textarea_create(localWifiPanel);
  lv_obj_set_size(taWifiPass, SCREEN_W - 40, 36);
  lv_obj_align(taWifiPass, LV_ALIGN_TOP_MID, 0, 100);
  styleDarkTextArea(taWifiPass);
  lv_textarea_set_one_line(taWifiPass, true);
  lv_textarea_set_password_mode(taWifiPass, true);
  lv_textarea_set_max_length(taWifiPass, 64);
  lv_textarea_set_placeholder_text(taWifiPass, "Password");
  lv_textarea_set_text(taWifiPass, wifiLocalPass);
  lv_obj_add_event_cb(taWifiPass, wifiInputEvent, LV_EVENT_FOCUSED, nullptr);

  lv_obj_t* btnSaveWifi = lv_btn_create(localWifiPanel);
  lv_obj_set_size(btnSaveWifi, SCREEN_W - 40, 40);
  lv_obj_align(btnSaveWifi, LV_ALIGN_TOP_MID, 0, 144);
  styleDarkObject(btnSaveWifi, COLOR_ACTION, 0xFFFFFF);
  lv_obj_t* lblSaveWifi = lv_label_create(btnSaveWifi);
  lv_label_set_text(lblSaveWifi, "Connect");
  styleBoundedLabel(lblSaveWifi, SCREEN_W - 56, 0x001B12);
  lv_obj_center(lblSaveWifi);
  lv_obj_add_event_cb(btnSaveWifi, [](lv_event_t*) {
    saveWifiCredentials();
  }, LV_EVENT_CLICKED, nullptr);
}

static void ensureWifiScanPage() {
  if (wifiScanPageBuilt || !pageWifiScan) return;
  wifiScanPageBuilt = true;

  makePageTitle(pageWifiScan, "Scan Networks");
  lblWifiScanStatus = lv_label_create(pageWifiScan);
  lv_label_set_text(lblWifiScanStatus, "Tap Refresh to scan");
  styleBoundedLabel(lblWifiScanStatus, SCREEN_W - 92, COLOR_MUTED);
  lv_obj_align(lblWifiScanStatus, LV_ALIGN_TOP_LEFT, 8, 24);
  lv_obj_t* btnRefreshScan = lv_btn_create(pageWifiScan);
  lv_obj_set_size(btnRefreshScan, 86, 30);
  lv_obj_align(btnRefreshScan, LV_ALIGN_TOP_RIGHT, -8, 20);
  styleDarkObject(btnRefreshScan, COLOR_ACTION, 0x001B12);
  lv_obj_t* lblRefreshScan = lv_label_create(btnRefreshScan);
  lv_label_set_text(lblRefreshScan, "Refresh");
  styleBoundedLabel(lblRefreshScan, 74, 0x001B12);
  lv_obj_center(lblRefreshScan);
  lv_obj_add_event_cb(btnRefreshScan, [](lv_event_t*) { requestWifiScan(); }, LV_EVENT_CLICKED, nullptr);
  listWifiScan = lv_list_create(pageWifiScan);
  lv_obj_set_size(listWifiScan, SCREEN_W - 12, 180);
  lv_obj_align(listWifiScan, LV_ALIGN_TOP_MID, 0, 58);
  styleDarkObject(listWifiScan, COLOR_PANEL);
}

static void processDeferredWifiAction() {
  if (!deferredWifiAction || millis() - deferredWifiActionMs < 50) return;
  uint8_t action = deferredWifiAction;
  deferredWifiAction = 0;
  if (action == 1) {
    ensureWifiLocalPage();
    showPage(pageWifiLocal);
  } else if (action == 2) {
    ensureWifiScanPage();
    showPage(pageWifiScan);
    requestWifiScan();
  }
}

static void buildLandscapeKeyboardScreen() {
  keyboardScreen = lv_obj_create(nullptr);
  styleDarkObject(keyboardScreen, COLOR_BG);
  lv_obj_set_style_border_width(keyboardScreen, 0, 0);
  lv_obj_clear_flag(keyboardScreen, LV_OBJ_FLAG_SCROLLABLE);

  keyboardPrompt = lv_label_create(keyboardScreen);
  lv_label_set_text(keyboardPrompt, "Message");
  styleBoundedLabel(keyboardPrompt, SCREEN_H - 16, COLOR_ACCENT);
  lv_obj_align(keyboardPrompt, LV_ALIGN_TOP_LEFT, 8, 5);

  keyboardText = lv_textarea_create(keyboardScreen);
  lv_obj_set_size(keyboardText, SCREEN_H - 16, 50);
  lv_obj_align(keyboardText, LV_ALIGN_TOP_MID, 0, 24);
  styleDarkTextArea(keyboardText);
  lv_textarea_set_one_line(keyboardText, true);
  lv_textarea_set_max_length(keyboardText, 233);
  lv_obj_add_event_cb(keyboardText, landscapeKeyboardEvent, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(keyboardText, landscapeKeyboardEvent, LV_EVENT_CANCEL, nullptr);

  landscapeKeyboard = lv_keyboard_create(keyboardScreen);
  lv_obj_set_size(landscapeKeyboard, SCREEN_H, 156);
  lv_obj_align(landscapeKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(landscapeKeyboard, COLOR_PANEL);
  lv_obj_set_style_bg_color(landscapeKeyboard, lv_color_hex(0x111A18), LV_PART_ITEMS);
  lv_obj_set_style_text_color(landscapeKeyboard, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_border_color(landscapeKeyboard, lv_color_hex(0x315B50), LV_PART_ITEMS);
  lv_obj_set_style_border_width(landscapeKeyboard, 1, LV_PART_ITEMS);
  lv_obj_add_event_cb(landscapeKeyboard, landscapeKeyboardEvent, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(landscapeKeyboard, landscapeKeyboardEvent, LV_EVENT_CANCEL, nullptr);
}

static void buildScreenUi() {
  lv_obj_t* screen = lv_scr_act();
  mainScreen = screen;
  styleDarkObject(screen, COLOR_BG);
  lv_obj_set_style_text_font(screen, UI_FONT_DEFAULT, 0);

  buildStatusBar(screen);

  pageLauncher = makePage(screen);
  pageOperate = makePage(screen);
  pageDiagnose = makePage(screen);
  pageNodes = makePage(screen);
  pageNodeDetail = makePage(screen);
  pageMeshHealth = makePage(screen);
  pagePacketInspector = makePage(screen);
  pageTxHistory = makePage(screen);
  pagePublicChat = makePage(screen);
  pagePrivateChat = makePage(screen);
  pageDirectChat = makePage(screen);
  pageGps = makePage(screen);
  pageSystem = makePage(screen);
  pageSystemInterface = makePage(screen);
  pageSystemSerial = makePage(screen);
  pageSystemRadio = makePage(screen);
  pageSystemGps = makePage(screen);
  pageWifi = makePage(screen);
  pageWifiStats = makePage(screen);
  pageWifiLocal = makePage(screen);
  pageWifiScan = makePage(screen);
  pageBacklight = makePage(screen);
  pageBattery = makePage(screen);

  currentPage = pageLauncher;
  lv_obj_clear_flag(pageLauncher, LV_OBJ_FLAG_HIDDEN);

  makeSystemTile(pageLauncher, "Messages", 0, 0, [](lv_event_t*) { showPage(pageOperate); });
  makeSystemTile(pageLauncher, "Nodes", 1, 0, [](lv_event_t*) {
    refreshNodeList(true);
    showPage(pageNodes);
  });
  makeSystemTile(pageLauncher, "Map", 0, 1, [](lv_event_t*) { showPage(pageGps); });
  makeSystemTile(pageLauncher, "System", 1, 1, [](lv_event_t*) { showPage(pageSystem); });
  makeSystemTile(pageLauncher, "TX Log", 0, 2, [](lv_event_t*) { showPage(pageTxHistory); });
  makeSystemTile(pageLauncher, "Health", 1, 2, [](lv_event_t*) { showPage(pageDiagnose); });

  makePageTitle(pageOperate, "Messages");
  makePageScrollable(pageOperate);
  lv_obj_t* publicTile = makeSystemTile(pageOperate, publicChannelLabel(), 0, 0, [](lv_event_t*) { showPage(pagePublicChat); });
  lblTilePublic = lv_obj_get_child(publicTile, 0);
  lv_obj_t* privateTile = makeSystemTile(pageOperate, privateChannelLabel(), 1, 0, [](lv_event_t*) { showPage(pagePrivateChat); });
  lblTilePrivate = lv_obj_get_child(privateTile, 0);
  makeSystemTile(pageOperate, "Direct", 0, 1, [](lv_event_t*) { showPage(pageDirectChat); });
  makeSystemTile(pageOperate, "TX Log", 1, 1, [](lv_event_t*) { showPage(pageTxHistory); });
  lblMsgPublic = makeDashboardLabel(pageOperate, 6, 120, 228, 34);
  lblMsgFamily = makeDashboardLabel(pageOperate, 6, 160, 228, 34);
  lblMsgDirect = makeDashboardLabel(pageOperate, 6, 200, 228, 34);

  makePageTitle(pageDiagnose, "Health");
  makePageScrollable(pageDiagnose);
  makeSystemTile(pageDiagnose, "Mesh", 0, 0, [](lv_event_t*) { showPage(pageMeshHealth); });
  makeSystemTile(pageDiagnose, "Packets", 1, 0, [](lv_event_t*) { showPage(pagePacketInspector); });
  makeSystemTile(pageDiagnose, "Serial", 0, 1, [](lv_event_t*) { showPage(pageSystemSerial); });
  makeSystemTile(pageDiagnose, "Radio", 1, 1, [](lv_event_t*) { showPage(pageSystemRadio); });
  makeSystemTile(pageDiagnose, "Interface", 0, 2, [](lv_event_t*) { showPage(pageSystemInterface); });
  makeSystemTile(pageDiagnose, "Power", 1, 2, [](lv_event_t*) { showPage(pageBattery); });
  makeSystemTile(pageDiagnose, "GPS", 0, 3, [](lv_event_t*) { showPage(pageSystemGps); });
  makeSystemTile(pageDiagnose, "WiFi", 1, 3, [](lv_event_t*) { showPage(pageWifiStats); });

  makePageTitle(pageNodes, "Nodes");
  makeSmallButton(pageNodes, "Sort", 6, 21, 48, [](lv_event_t*) { cycleNodeSort(); });
  makeSmallButton(pageNodes, "Filter", 60, 21, 56, [](lv_event_t*) { cycleNodeFilter(); });
  makeSmallButton(pageNodes, "Map", 122, 21, 46, [](lv_event_t*) { showPage(pageGps); });
  makeSmallButton(pageNodes, "Clear", 174, 21, 60, [](lv_event_t*) { clearNodeList(); });
  lblNodeListMode = lv_label_create(pageNodes);
  lv_label_set_text(lblNodeListMode, "sort heard  filter all");
  lv_obj_set_width(lblNodeListMode, SCREEN_W - 12);
  lv_label_set_long_mode(lblNodeListMode, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_color(lblNodeListMode, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(lblNodeListMode, UI_FONT_SMALL, 0);
  lv_obj_align(lblNodeListMode, LV_ALIGN_TOP_LEFT, 6, 46);
  listNodes = lv_list_create(pageNodes);
  lv_obj_set_size(listNodes, SCREEN_W - 12, 188);
  lv_obj_align(listNodes, LV_ALIGN_TOP_MID, 0, 62);
  styleDarkObject(listNodes, COLOR_PANEL);
  styleDarkBorder(listNodes, 0x2F705F);
  lv_obj_set_style_pad_all(listNodes, 4, 0);
  lv_obj_set_style_pad_row(listNodes, 6, 0);

  makePageTitle(pageNodeDetail, "Node Detail");
  taNodeDetail = makeReadonlyText(pageNodeDetail, 22, 184);
  makeSmallButton(pageNodeDetail, "Message", 6, 214, 108, [](lv_event_t*) { directSelectedNode(); });
  makeSmallButton(pageNodeDetail, "Map", 126, 214, 108, [](lv_event_t*) { showSelectedNodeOnMap(); });

  makePageTitle(pageMeshHealth, "Mesh Health");
  makePageScrollable(pageMeshHealth);
  lv_obj_t* meshHealthPanel = makePanel(pageMeshHealth);
  lv_obj_set_size(meshHealthPanel, SCREEN_W - 12, 392);
  lv_obj_align(meshHealthPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblMeshHealth = lv_label_create(meshHealthPanel);
  lv_label_set_text(lblMeshHealth, "Waiting for radio health...");
  styleBoundedLabel(lblMeshHealth, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pagePacketInspector, "Packets");
  taPacketInspector = makeReadonlyText(pagePacketInspector, 22, 226);
  lv_textarea_set_text(taPacketInspector, "No packets decoded yet");

  makePageTitle(pageTxHistory, "TX History");
  taTxHistory = makeReadonlyText(pageTxHistory, 22, 184);
  setReadonlyTextStable(taTxHistory, "No messages sent yet");
  makeActionButton(pageTxHistory, "Retry Last", 212, [](lv_event_t*) { retryLastTx(); });

  lblPublicChatTitle = makePageTitle(pagePublicChat, "Primary Chat");
  lblPublicChannelStats = lv_label_create(pagePublicChat);
  lv_label_set_text(lblPublicChannelStats, "Ch 0: primary");
  styleBoundedLabel(lblPublicChannelStats, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPublicChannelStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taPublicChat = makeReadonlyText(pagePublicChat, 32, 154);
  lv_textarea_set_text(taPublicChat, "No public chat yet");
  taPublicInput = lv_textarea_create(pagePublicChat);
  lv_obj_set_size(taPublicInput, SCREEN_W - 76, 38);
  lv_obj_align(taPublicInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taPublicInput);
  lv_textarea_set_one_line(taPublicInput, true);
  lv_textarea_set_max_length(taPublicInput, 233);
  lv_textarea_set_placeholder_text(taPublicInput, "Primary message");
  lv_textarea_set_text(taPublicInput, "");
  lv_obj_add_event_cb(taPublicInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* publicSendBtn = lv_btn_create(pagePublicChat);
  lv_obj_set_size(publicSendBtn, 56, 38);
  lv_obj_align(publicSendBtn, LV_ALIGN_TOP_RIGHT, -6, 192);
  styleDarkObject(publicSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(publicSendBtn, 0, 0);
  lv_obj_add_event_cb(publicSendBtn, [](lv_event_t*) { sendFromInput(taPublicInput, PUBLIC_CHANNEL_INDEX); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(publicSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* publicSendLbl = lv_label_create(publicSendBtn);
  lv_label_set_text(publicSendLbl, "Send");
  lv_obj_set_style_text_color(publicSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(publicSendLbl);
  lv_obj_move_foreground(publicSendBtn);

  lblPrivateChatTitle = makePageTitle(pagePrivateChat, "Channel 1 Chat");
  lblPrivateChannelStats = lv_label_create(pagePrivateChat);
  lv_label_set_text(lblPrivateChannelStats, "Ch 1");
  styleBoundedLabel(lblPrivateChannelStats, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPrivateChannelStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taFamilyChat = makeReadonlyText(pagePrivateChat, 32, 154);
  lv_textarea_set_text(taFamilyChat, "No secondary chat yet");
  taFamilyInput = lv_textarea_create(pagePrivateChat);
  lv_obj_set_size(taFamilyInput, SCREEN_W - 76, 38);
  lv_obj_align(taFamilyInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taFamilyInput);
  lv_textarea_set_one_line(taFamilyInput, true);
  lv_textarea_set_max_length(taFamilyInput, 233);
  lv_textarea_set_placeholder_text(taFamilyInput, "Channel message");
  lv_textarea_set_text(taFamilyInput, "");
  lv_obj_add_event_cb(taFamilyInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* privateSendBtn = lv_btn_create(pagePrivateChat);
  lv_obj_set_size(privateSendBtn, 56, 38);
  lv_obj_align(privateSendBtn, LV_ALIGN_TOP_RIGHT, -6, 192);
  styleDarkObject(privateSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(privateSendBtn, 0, 0);
  lv_obj_add_event_cb(privateSendBtn, [](lv_event_t*) { sendFromInput(taFamilyInput, privateChannelIndex >= 0 ? privateChannelIndex : 1); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(privateSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* privateSendLbl = lv_label_create(privateSendBtn);
  lv_label_set_text(privateSendLbl, "Send");
  lv_obj_set_style_text_color(privateSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(privateSendLbl);
  lv_obj_move_foreground(privateSendBtn);

  lblPrivateChannelHint = lv_label_create(pagePrivateChat);
  lv_label_set_text(lblPrivateChannelHint, "Radio ch: not loaded");
  styleBoundedLabel(lblPrivateChannelHint, SCREEN_W - 12, COLOR_MUTED);
  lv_obj_align(lblPrivateChannelHint, LV_ALIGN_TOP_LEFT, 2, 236);

  makePageTitle(pageDirectChat, "Direct Messages");
  taDirectChat = makeReadonlyText(pageDirectChat, 22, 132);
  lv_textarea_set_text(taDirectChat, "No direct messages yet");
  taDirectTo = lv_textarea_create(pageDirectChat);
  lv_obj_set_size(taDirectTo, SCREEN_W - 12, 34);
  lv_obj_align(taDirectTo, LV_ALIGN_TOP_LEFT, 6, 158);
  styleDarkTextArea(taDirectTo);
  lv_textarea_set_one_line(taDirectTo, true);
  lv_textarea_set_max_length(taDirectTo, 9);
  lv_textarea_set_placeholder_text(taDirectTo, "To: !1234ABCD");
  lv_textarea_set_text(taDirectTo, "");
  lv_obj_add_event_cb(taDirectTo, inputEvent, LV_EVENT_ALL, nullptr);
  taDirectInput = lv_textarea_create(pageDirectChat);
  lv_obj_set_size(taDirectInput, SCREEN_W - 76, 38);
  lv_obj_align(taDirectInput, LV_ALIGN_TOP_LEFT, 6, 198);
  styleDarkTextArea(taDirectInput);
  lv_textarea_set_one_line(taDirectInput, true);
  lv_textarea_set_max_length(taDirectInput, 233);
  lv_textarea_set_placeholder_text(taDirectInput, "Direct message");
  lv_textarea_set_text(taDirectInput, "");
  lv_obj_add_event_cb(taDirectInput, inputEvent, LV_EVENT_ALL, nullptr);
  lv_obj_t* directSendBtn = lv_btn_create(pageDirectChat);
  lv_obj_set_size(directSendBtn, 56, 38);
  lv_obj_align(directSendBtn, LV_ALIGN_TOP_RIGHT, -6, 198);
  styleDarkObject(directSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(directSendBtn, 0, 0);
  lv_obj_add_event_cb(directSendBtn, [](lv_event_t*) { sendDirectFromInput(taDirectTo, taDirectInput); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(directSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* directSendLbl = lv_label_create(directSendBtn);
  lv_label_set_text(directSendLbl, "Send");
  lv_obj_set_style_text_color(directSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(directSendLbl);
  lv_obj_move_foreground(directSendBtn);

  makePageTitle(pageGps, "Map");
  makePageScrollable(pageGps);
  lv_obj_t* btnMapHome = lv_btn_create(pageGps);
  lv_obj_set_size(btnMapHome, 50, 22);
  lv_obj_align(btnMapHome, LV_ALIGN_TOP_RIGHT, -58, 0);
  styleDarkObject(btnMapHome, COLOR_PANEL);
  styleDarkBorder(btnMapHome, 0x2F705F);
  lv_obj_set_style_radius(btnMapHome, 6, 0);
  lv_obj_set_style_shadow_width(btnMapHome, 0, 0);
  lv_obj_add_event_cb(btnMapHome, [](lv_event_t*) {
    setMapZoomedOut(false);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lblMapHome = lv_label_create(btnMapHome);
  lv_label_set_text(lblMapHome, "Zoom");
  lv_obj_set_style_text_color(lblMapHome, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(lblMapHome);

  lv_obj_t* btnMapWide = lv_btn_create(pageGps);
  lv_obj_set_size(btnMapWide, 50, 22);
  lv_obj_align(btnMapWide, LV_ALIGN_TOP_RIGHT, -2, 0);
  styleDarkObject(btnMapWide, COLOR_PANEL);
  styleDarkBorder(btnMapWide, 0x2F705F);
  lv_obj_set_style_radius(btnMapWide, 6, 0);
  lv_obj_set_style_shadow_width(btnMapWide, 0, 0);
  lv_obj_add_event_cb(btnMapWide, [](lv_event_t*) { setMapZoomedOut(true); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lblMapWide = lv_label_create(btnMapWide);
  lv_label_set_text(lblMapWide, "Wide");
  lv_obj_set_style_text_color(lblMapWide, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(lblMapWide);

  mapPlot = makePanel(pageGps);
  lv_obj_set_size(mapPlot, MAP_PLOT_W, activeMapPlotHeight());
  lv_obj_align(mapPlot, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_bg_color(mapPlot, lv_color_hex(COLOR_INPUT), 0);
  lv_obj_set_style_pad_all(mapPlot, 0, 0);
  lv_obj_clear_flag(mapPlot, LV_OBJ_FLAG_SCROLLABLE);
  if (mapCanvasBuf) {
    mapCanvas = lv_canvas_create(mapPlot);
    lv_canvas_set_buffer(mapCanvas, mapCanvasBuf, MAP_PLOT_W, MAP_PLOT_H, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
    lv_obj_set_pos(mapCanvas, 0, 0);
    lv_obj_clear_flag(mapCanvas, LV_OBJ_FLAG_CLICKABLE);
  }
  for (size_t i = 0; i < MAP_DOT_COUNT; i++) {
    mapDots[i] = lv_obj_create(mapPlot);
    styleDarkObject(mapDots[i], COLOR_ACCENT);
    lv_obj_set_size(mapDots[i], 8, 8);
    lv_obj_set_style_radius(mapDots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(mapDots[i], 0, 0);
    lv_obj_set_style_bg_color(mapDots[i], lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_add_flag(mapDots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(mapDots[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(mapDots[i], LV_OBJ_FLAG_CLICKABLE);
  }
  lblMapStats = lv_label_create(pageGps);
  lv_label_set_text(lblMapStats, "Waiting for node positions...");
  lv_obj_set_style_text_color(lblMapStats, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblMapStats, SCREEN_W - 16);
  lv_label_set_long_mode(lblMapStats, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblMapStats, LV_ALIGN_TOP_LEFT, 6, 164);

  makePageTitle(pageSystem, "System");
  makePageScrollable(pageSystem);
  makeSystemTile(pageSystem, "Interface", 0, 0, [](lv_event_t*) { showPage(pageSystemInterface); });
  makeSystemTile(pageSystem, "Serial", 1, 0, [](lv_event_t*) { showPage(pageSystemSerial); });
  makeSystemTile(pageSystem, "Radio", 0, 1, [](lv_event_t*) { showPage(pageSystemRadio); });
  makeSystemTile(pageSystem, "GPS", 1, 1, [](lv_event_t*) { showPage(pageSystemGps); });
  makeSystemTile(pageSystem, "WiFi", 0, 2, [](lv_event_t*) { showPage(pageWifi); });
  makeSystemTile(pageSystem, "Display", 1, 2, [](lv_event_t*) { showPage(pageBacklight); });
  makeSystemTile(pageSystem, "Battery", 0, 3, [](lv_event_t*) { showPage(pageBattery); });

  makePageTitle(pageSystemInterface, "Interface");
  makePageScrollable(pageSystemInterface);
  lv_obj_t* interfacePanel = makePanel(pageSystemInterface);
  lv_obj_set_size(interfacePanel, SCREEN_W - 12, 330);
  lv_obj_align(interfacePanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemInterface = lv_label_create(interfacePanel);
  lv_label_set_text(lblSystemInterface, "Interface ready");
  styleBoundedLabel(lblSystemInterface, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemSerial, "Serial Link");
  makePageScrollable(pageSystemSerial);
  lv_obj_t* serialPanel = makePanel(pageSystemSerial);
  lv_obj_set_size(serialPanel, SCREEN_W - 12, 300);
  lv_obj_align(serialPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemSerial = lv_label_create(serialPanel);
  lv_label_set_text(lblSystemSerial, "Serial ready");
  styleBoundedLabel(lblSystemSerial, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemRadio, "Radio Stats");
  makePageScrollable(pageSystemRadio);
  lv_obj_t* radioPanel = makePanel(pageSystemRadio);
  lv_obj_set_size(radioPanel, SCREEN_W - 12, 300);
  lv_obj_align(radioPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemRadio = lv_label_create(radioPanel);
  lv_label_set_text(lblSystemRadio, "Radio ready");
  styleBoundedLabel(lblSystemRadio, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageSystemGps, "GPS Stats");
  makePageScrollable(pageSystemGps);
  lv_obj_t* gpsPanel = makePanel(pageSystemGps);
  lv_obj_set_size(gpsPanel, SCREEN_W - 12, 430);
  lv_obj_align(gpsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblGpsStats = lv_label_create(gpsPanel);
  lv_label_set_text(lblGpsStats, "Waiting for CYD GPS UART...");
  styleBoundedLabel(lblGpsStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageWifi, "WiFi");
  makePageScrollable(pageWifi);
  lv_obj_t* wifiPanel = makePanel(pageWifi);
  lv_obj_set_size(wifiPanel, UI_PANEL_W, 142);
  lv_obj_align(wifiPanel, LV_ALIGN_TOP_MID, 0, 26);
  lv_obj_t* wifiToggleLabel = lv_label_create(wifiPanel);
  lv_label_set_text(wifiToggleLabel, "WiFi On/Off");
  styleBoundedLabel(wifiToggleLabel, 118, COLOR_TEXT);
  lv_obj_align(wifiToggleLabel, LV_ALIGN_TOP_LEFT, 2, 4);
  swWifiEnabled = lv_switch_create(wifiPanel);
  lv_obj_align(swWifiEnabled, LV_ALIGN_TOP_RIGHT, -2, 0);
  if (wifiEnabled) lv_obj_add_state(swWifiEnabled, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swWifiEnabled, [](lv_event_t* e) {
    setWifiEnabled(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* wifiModeLabel = lv_label_create(wifiPanel);
  lv_label_set_text(wifiModeLabel, "Local / AP");
  styleBoundedLabel(wifiModeLabel, 118, COLOR_TEXT);
  lv_obj_align(wifiModeLabel, LV_ALIGN_TOP_LEFT, 2, 42);
  swWifiApMode = lv_switch_create(wifiPanel);
  lv_obj_align(swWifiApMode, LV_ALIGN_TOP_RIGHT, -2, 38);
  if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swWifiApMode, [](lv_event_t* e) {
    setWifiApMode(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lblWifiState = lv_label_create(wifiPanel);
  lv_label_set_text(lblWifiState, "WiFi starting...");
  styleBoundedLabel(lblWifiState, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblWifiState, LV_ALIGN_TOP_LEFT, 2, 80);
  makeActionButton(pageWifi, "Local Network", 170, [](lv_event_t*) {
    deferWifiAction(1);
  });
  makeActionButton(pageWifi, "WiFi Stats", 214, [](lv_event_t*) { showPage(pageWifiStats); });

  makePageTitle(pageWifiStats, "WiFi Stats");
  makePageScrollable(pageWifiStats);
  lv_obj_t* wifiStatsPanel = makePanel(pageWifiStats);
  lv_obj_set_size(wifiStatsPanel, SCREEN_W - 12, 330);
  lv_obj_align(wifiStatsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblWifiStats = lv_label_create(wifiStatsPanel);
  lv_label_set_text(lblWifiStats, "WiFi stats unavailable");
  styleBoundedLabel(lblWifiStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  makePageTitle(pageBacklight, "Display");
  lv_obj_t* backlightPanel = makePanel(pageBacklight);
  lv_obj_set_size(backlightPanel, SCREEN_W - 12, 206);
  lv_obj_align(backlightPanel, LV_ALIGN_TOP_MID, 0, 30);
  lblBacklight = lv_label_create(backlightPanel);
  lv_label_set_text(lblBacklight, "Brightness: 10%");
  styleBoundedLabel(lblBacklight, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblBacklight, LV_ALIGN_TOP_LEFT, 2, 4);
  sliderBacklight = lv_slider_create(backlightPanel);
  lv_obj_set_size(sliderBacklight, SCREEN_W - 42, 24);
  lv_obj_align(sliderBacklight, LV_ALIGN_TOP_MID, 0, 54);
  lv_slider_set_range(sliderBacklight, 10, 100);
  lv_slider_set_value(sliderBacklight, backlightPercent, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(0x16342C), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(COLOR_ACTION), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sliderBacklight, lv_color_hex(COLOR_TEXT), LV_PART_KNOB);
  lv_obj_add_event_cb(sliderBacklight, backlightSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_t* readableLabel = lv_label_create(backlightPanel);
  lv_label_set_text(readableLabel, "Readable Text");
  styleBoundedLabel(readableLabel, 118, COLOR_TEXT);
  lv_obj_align(readableLabel, LV_ALIGN_TOP_LEFT, 2, 92);
  swReadableText = lv_switch_create(backlightPanel);
  lv_obj_align(swReadableText, LV_ALIGN_TOP_RIGHT, -2, 86);
  if (readableTextMode) lv_obj_add_state(swReadableText, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swReadableText, [](lv_event_t* e) {
    setReadableTextMode(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED), true);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lblTextMode = lv_label_create(backlightPanel);
  lv_label_set_text(lblTextMode, readableTextMode ? "Readable font" : "Compact font");
  styleBoundedLabel(lblTextMode, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(lblTextMode, LV_ALIGN_TOP_LEFT, 2, 126);
  lv_obj_t* backlightHint = lv_label_create(backlightPanel);
  lv_label_set_text(backlightHint, "Dimmer saves battery.");
  styleBoundedLabel(backlightHint, lv_pct(100), COLOR_MUTED, LV_LABEL_LONG_WRAP);
  lv_obj_align(backlightHint, LV_ALIGN_TOP_LEFT, 2, 164);

  makePageTitle(pageBattery, "Battery");
  makePageScrollable(pageBattery);
  lv_obj_t* batteryPanel = makePanel(pageBattery);
  lv_obj_set_size(batteryPanel, SCREEN_W - 12, 470);
  lv_obj_align(batteryPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblBatteryStats = lv_label_create(batteryPanel);
  lv_label_set_text(lblBatteryStats, "Reading S3 battery...");
  styleBoundedLabel(lblBatteryStats, lv_pct(100), COLOR_TEXT, LV_LABEL_LONG_WRAP);

  buildNavBar(screen);

  keyboard = lv_keyboard_create(screen);
  lv_obj_set_size(keyboard, SCREEN_W, 132);
  lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  styleDarkObject(keyboard, COLOR_PANEL);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x111A18), LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_border_color(keyboard, lv_color_hex(0x315B50), LV_PART_ITEMS);
  lv_obj_set_style_border_width(keyboard, 1, LV_PART_ITEMS);
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  refreshChannelUiLabels();
}

static void rebuildScreenUi(lv_obj_t* targetPage) {
  if (!display) return;
  lv_obj_t* screen = lv_scr_act();
  if (!screen) return;
  if (keyboardScreen) {
    lv_obj_del(keyboardScreen);
    keyboardScreen = nullptr;
    keyboardPrompt = nullptr;
    keyboardText = nullptr;
    landscapeKeyboard = nullptr;
    landscapeKeyboardOpen = false;
  }
  lv_obj_clean(screen);
  clearUiObjectPointers();
  buildScreenUi();
  buildLandscapeKeyboardScreen();
  if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  if (taWifiPass) lv_textarea_set_text(taWifiPass, wifiLocalPass);
  if (swWifiApMode) {
    if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiApMode, LV_STATE_CHECKED);
  }
  if (targetPage) showPage(targetPage, false);
  refreshDashboardLabels();
  refreshScreenUi();
}

static void setReadableTextMode(bool enabled, bool save) {
  if (readableTextMode == enabled && !save) return;
  readableTextMode = enabled;
  applyFontMode();
  if (save) prefs.putBool("readableText", readableTextMode);
  if (lblTextMode) {
    lv_label_set_text(lblTextMode, readableTextMode ? "Readable font" : "Compact font");
  }
  if (swReadableText) {
    if (readableTextMode) lv_obj_add_state(swReadableText, LV_STATE_CHECKED);
    else lv_obj_clear_state(swReadableText, LV_STATE_CHECKED);
  }
  if (mainScreen) {
    rebuildScreenUi(nullptr);
    showPage(pageBacklight, false);
  }
}

static void initScreen() {
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);

  initBacklight();

  lv_init();
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, SCREEN_W * 24);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_W;
  dispDrv.ver_res = SCREEN_H;
  dispDrv.flush_cb = lvFlush;
  dispDrv.draw_buf = &drawBuf;
  display = lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvTouchRead;
  lv_indev_drv_register(&indevDrv);

  buildScreenUi();
  buildLandscapeKeyboardScreen();
}

static size_t countPositionedNodes() {
  size_t count = 0;
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].hasPosition) count++;
  }
  return count;
}

static double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double r = 6371000.0;
  double p1 = lat1 * DEG_TO_RAD;
  double p2 = lat2 * DEG_TO_RAD;
  double dp = (lat2 - lat1) * DEG_TO_RAD;
  double dl = (lon2 - lon1) * DEG_TO_RAD;
  double a = sin(dp / 2.0) * sin(dp / 2.0) +
             cos(p1) * cos(p2) * sin(dl / 2.0) * sin(dl / 2.0);
  return r * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

static bool updateNodePosition(uint32_t nodeNum, const meshtastic_Position& position, const char* source) {
  if (!position.has_latitude_i || !position.has_longitude_i) return false;
  if (position.latitude_i == 0 && position.longitude_i == 0) return false;

  NodeRecord* node = findOrCreateNode(nodeNum);
  if (!node) return false;
  node->hasPosition = true;
  node->latitude = position.latitude_i / 10000000.0;
  node->longitude = position.longitude_i / 10000000.0;
  node->altitude = position.has_altitude ? position.altitude : 0;
  node->lastPositionMs = millis();

  char line[128];
  snprintf(line, sizeof(line), "[map] %s %.5f, %.5f\n", nodeName(nodeNum), node->latitude, node->longitude);
  appendLine(eventLog, LOG_SIZE, line);
  logPositionToSd(nodeNum, source ? source : "meshtastic", node->latitude, node->longitude, node->altitude);
  if (stats.myNodeNum != 0 && nodeNum == stats.myNodeNum) {
    saveLastLocationToSd(node->latitude, node->longitude, node->altitude);
  }
  markUiDirty(UI_DIRTY_MAP | UI_DIRTY_NODES | UI_DIRTY_NODE_DETAIL | UI_DIRTY_DASHBOARD | UI_DIRTY_SYSTEM);
  mapRenderPending = true;
  return true;
}

static long lonToTileX(double lon, int zoom) {
  double n = (double)(1UL << zoom);
  return (long)floor((lon + 180.0) / 360.0 * n);
}

static double lonToGlobalPixelX(double lon, int zoom) {
  double n = (double)(1UL << zoom) * MAP_TILE_SIZE;
  return (lon + 180.0) / 360.0 * n;
}

static long latToTileY(double lat, int zoom) {
  lat = constrain(lat, -85.05112878, 85.05112878);
  double latRad = lat * DEG_TO_RAD;
  double n = (double)(1UL << zoom);
  return (long)floor((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n);
}

static double latToGlobalPixelY(double lat, int zoom) {
  lat = constrain(lat, -85.05112878, 85.05112878);
  double latRad = lat * DEG_TO_RAD;
  double n = (double)(1UL << zoom) * MAP_TILE_SIZE;
  return (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n;
}

static bool mapTileExists(int zoom, long x, long y, char* path, size_t pathSize) {
  buildTilePath(mapTileRoot, zoom, x, y, path, pathSize);
  return sdStorage.available && SD_MMC.exists(path);
}

static int findBestMapZoom(double lat, double lon, char* centerPath, size_t centerPathSize) {
  for (int zoom = MAP_TILE_MAX_ZOOM; zoom >= MAP_TILE_MIN_ZOOM; zoom--) {
    long tileX = lonToTileX(lon, zoom);
    long tileY = latToTileY(lat, zoom);
    if (mapTileExists(zoom, tileX, tileY, centerPath, centerPathSize)) {
      return zoom;
    }
  }
  long tileX = lonToTileX(lon, MAP_TILE_MAX_ZOOM);
  long tileY = latToTileY(lat, MAP_TILE_MAX_ZOOM);
  mapTileExists(MAP_TILE_MAX_ZOOM, tileX, tileY, centerPath, centerPathSize);
  return MAP_TILE_MAX_ZOOM;
}

static lv_color_t rgb565ToLvColor(uint16_t rgb565) {
  return lv_color_make((rgb565 >> 8) & 0xF8, (rgb565 >> 3) & 0xFC, (rgb565 << 3) & 0xF8);
}

static void loadMapCacheFromSd() {
  if (!sdStorage.available || !mapCanvas) {
    strlcpy(mapCacheStatus, "not ready", sizeof(mapCacheStatus));
    return;
  }
  bool hasLastLocation = loadLastLocationFromSd();
  if (!SD_MMC.exists(SD_MAP_CACHE_PATH)) {
    strlcpy(mapCacheStatus, hasLastLocation ? "image missing, location loaded" : "missing", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s path=%s\n", mapCacheStatus, SD_MAP_CACHE_PATH);
    return;
  }
  File file = SD_MMC.open(SD_MAP_CACHE_PATH, FILE_READ);
  if (!file) {
    strlcpy(mapCacheStatus, "open failed", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s path=%s\n", mapCacheStatus, SD_MAP_CACHE_PATH);
    return;
  }

  MapCacheHeader header;
  if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
    file.close();
    strlcpy(mapCacheStatus, "header read failed", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s path=%s\n", mapCacheStatus, SD_MAP_CACHE_PATH);
    return;
  }
  if (header.magic != MAP_CACHE_MAGIC ||
      header.version != MAP_CACHE_VERSION ||
      header.width != MAP_PLOT_W ||
      (header.height != MAP_PLOT_H_NORMAL && header.height != MAP_PLOT_H_EXPANDED)) {
    file.close();
    strlcpy(mapCacheStatus, "header mismatch", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s magic=%08lX ver=%u size=%ux%u expected=%ux%u\n",
                  mapCacheStatus,
                  (unsigned long)header.magic,
                  (unsigned)header.version,
                  (unsigned)header.width,
                  (unsigned)header.height,
                  (unsigned)MAP_PLOT_W,
                  (unsigned)MAP_PLOT_H_EXPANDED);
    return;
  }
  size_t expected = MAP_CANVAS_BYTES;
  if (file.read((uint8_t*)mapCanvasBuf, expected) != expected) {
    file.close();
    strlcpy(mapCacheStatus, "image read failed", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s path=%s\n", mapCacheStatus, SD_MAP_CACHE_PATH);
    return;
  }
  file.close();

  cachedMapZoom = header.zoom;
  cachedMapTileX = header.tileX;
  cachedMapTileY = header.tileY;
  cachedMapPixelX = header.pixelX;
  cachedMapPixelY = header.pixelY;
  cachedMapLat = header.lat;
  cachedMapLon = header.lon;
  cachedMapHeight = header.height;
  cachedMapTileFound = header.tileFound != 0;
  mapCanvasCached = true;
  strlcpy(mapCacheStatus, "loaded", sizeof(mapCacheStatus));
  Serial.printf("[MAPCACHE] loaded %.6f,%.6f z%d tile=%ld/%ld found=%s bytes=%u\n",
                cachedMapLat,
                cachedMapLon,
                cachedMapZoom,
                cachedMapTileX,
                cachedMapTileY,
                cachedMapTileFound ? "true" : "false",
                (unsigned)expected);
  lv_obj_invalidate(mapCanvas);
}

static void saveMapCacheToSd() {
  if (!sdStorage.available || !mapCanvasCached) {
    strlcpy(mapCacheStatus, "save skipped", sizeof(mapCacheStatus));
    return;
  }
  if (SD_MMC.exists(SD_MAP_CACHE_PATH)) SD_MMC.remove(SD_MAP_CACHE_PATH);
  File file = SD_MMC.open(SD_MAP_CACHE_PATH, FILE_WRITE);
  if (!file) {
    strlcpy(mapCacheStatus, "save open failed", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s path=%s\n", mapCacheStatus, SD_MAP_CACHE_PATH);
    return;
  }
  MapCacheHeader header = {};
  header.magic = MAP_CACHE_MAGIC;
  header.version = MAP_CACHE_VERSION;
  header.width = MAP_PLOT_W;
  header.height = activeMapPlotHeight();
  header.zoom = cachedMapZoom;
  header.tileX = cachedMapTileX;
  header.tileY = cachedMapTileY;
  header.pixelX = cachedMapPixelX;
  header.pixelY = cachedMapPixelY;
  header.lat = cachedMapLat;
  header.lon = cachedMapLon;
  header.tileFound = cachedMapTileFound ? 1 : 0;
  size_t headerWritten = file.write((const uint8_t*)&header, sizeof(header));
  size_t imageWritten = file.write((const uint8_t*)mapCanvasBuf, MAP_CANVAS_BYTES);
  file.close();
  strlcpy(mapCacheStatus, imageWritten == MAP_CANVAS_BYTES ? "saved" : "save short write", sizeof(mapCacheStatus));
  Serial.printf("[MAPCACHE] %s %.6f,%.6f z%d tile=%ld/%ld header=%u image=%u\n",
                mapCacheStatus,
                cachedMapLat,
                cachedMapLon,
                cachedMapZoom,
                cachedMapTileX,
                cachedMapTileY,
                (unsigned)headerWritten,
                (unsigned)imageWritten);
}

static bool renderOfflineTileMap(double lat, double lon, int zoom, char* centerPath, size_t centerPathSize) {
  if (!mapCanvas || !sdStorage.available || !mapTileRootFound) return false;
  int plotH = activeMapPlotHeight();

  double centerX = lonToGlobalPixelX(lon, zoom);
  double centerY = latToGlobalPixelY(lat, zoom);
  long centerTileX = (long)floor(centerX / MAP_TILE_SIZE);
  long centerTileY = (long)floor(centerY / MAP_TILE_SIZE);
  bool centerTileFound = mapTileExists(zoom, centerTileX, centerTileY, centerPath, centerPathSize);
  if (!centerTileFound) {
    cachedMapZoom = zoom;
    cachedMapTileX = centerTileX;
    cachedMapTileY = centerTileY;
    cachedMapLat = lat;
    cachedMapLon = lon;
    cachedMapTileFound = false;
    if (!mapCanvasCached) lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
    strlcpy(mapCacheStatus, "center tile missing", sizeof(mapCacheStatus));
    return false;
  }

  int centerPixelX = (int)round(centerX);
  int centerPixelY = (int)round(centerY);
  if (mapCanvasCached &&
      cachedMapZoom == zoom &&
      cachedMapTileX == centerTileX &&
      cachedMapTileY == centerTileY &&
      abs(cachedMapPixelX - centerPixelX) < 32 &&
      abs(cachedMapPixelY - centerPixelY) < 32 &&
      cachedMapHeight == plotH &&
      cachedMapTileFound == centerTileFound) {
    return centerTileFound;
  }

  int startX = centerPixelX - (MAP_PLOT_W / 2);
  int startY = centerPixelY - (plotH / 2);
  int endX = startX + MAP_PLOT_W - 1;
  int endY = startY + plotH - 1;

  for (int y = 0; y < plotH; y++) {
    for (int x = 0; x < MAP_PLOT_W; x++) {
      mapCanvasBuf[y * MAP_PLOT_W + x] = lv_color_hex(0x07100D);
    }
  }

  long tileX0 = max(0L, (long)floor((double)startX / MAP_TILE_SIZE));
  long tileY0 = max(0L, (long)floor((double)startY / MAP_TILE_SIZE));
  long tileX1 = max(0L, (long)floor((double)endX / MAP_TILE_SIZE));
  long tileY1 = max(0L, (long)floor((double)endY / MAP_TILE_SIZE));

  for (long tileY = tileY0; tileY <= tileY1; tileY++) {
    for (long tileX = tileX0; tileX <= tileX1; tileX++) {
      char path[96];
      buildTilePath(mapTileRoot, zoom, tileX, tileY, path, sizeof(path));
      File tile = SD_MMC.open(path, FILE_READ);
      if (!tile) continue;

      int tileGlobalX0 = tileX * MAP_TILE_SIZE;
      int tileGlobalY0 = tileY * MAP_TILE_SIZE;
      int overlapX0 = max(startX, tileGlobalX0);
      int overlapY0 = max(startY, tileGlobalY0);
      int overlapX1 = min(endX, tileGlobalX0 + MAP_TILE_SIZE - 1);
      int overlapY1 = min(endY, tileGlobalY0 + MAP_TILE_SIZE - 1);
      int segment = overlapX1 - overlapX0 + 1;
      int outX = overlapX0 - startX;
      int inTileX = overlapX0 - tileGlobalX0;
      for (int globalY = overlapY0; globalY <= overlapY1; globalY++) {
        int outY = globalY - startY;
        int inTileY = globalY - tileGlobalY0;
        size_t bytesToRead = segment * sizeof(uint16_t);
        tile.seek((inTileY * MAP_TILE_SIZE + inTileX) * sizeof(uint16_t));
        size_t bytesRead = tile.read((uint8_t*)mapReadBuf, bytesToRead);
        int pixelsRead = bytesRead / sizeof(uint16_t);
        for (int i = 0; i < pixelsRead; i++) {
          mapCanvasBuf[outY * MAP_PLOT_W + outX + i] = rgb565ToLvColor(mapReadBuf[i]);
        }
        if ((globalY & 0x0F) == 0) delay(0);
      }
      tile.close();
      delay(0);
    }
  }

  cachedMapZoom = zoom;
  cachedMapTileX = centerTileX;
  cachedMapTileY = centerTileY;
  cachedMapPixelX = centerPixelX;
  cachedMapPixelY = centerPixelY;
  cachedMapHeight = activeMapPlotHeight();
  cachedMapLat = lat;
  cachedMapLon = lon;
  cachedMapTileFound = centerTileFound;
  mapCanvasCached = true;
  strlcpy(mapCacheStatus, "drawn", sizeof(mapCacheStatus));
  if (!mapNodeDetailViewActive && (!lastMapCacheSaveMs || millis() - lastMapCacheSaveMs > 15000)) {
    saveMapCacheToSd();
    lastMapCacheSaveMs = millis();
  }
  lv_obj_invalidate(mapCanvas);
  return centerTileFound;
}

static bool placeMapDot(size_t index, double centerLat, double centerLon, double lat, double lon, int zoom, lv_color_t color, int size) {
  if (index >= MAP_DOT_COUNT || !mapDots[index]) return false;
  int plotH = activeMapPlotHeight();
  double centerX = lonToGlobalPixelX(centerLon, zoom);
  double centerY = latToGlobalPixelY(centerLat, zoom);
  double pointX = lonToGlobalPixelX(lon, zoom);
  double pointY = latToGlobalPixelY(lat, zoom);
  int x = (MAP_PLOT_W / 2) + (int)round(pointX - centerX) - (size / 2);
  int y = (plotH / 2) + (int)round(pointY - centerY) - (size / 2);
  if (x < -size || y < -size || x > MAP_PLOT_W || y > plotH) {
    lv_obj_add_flag(mapDots[index], LV_OBJ_FLAG_HIDDEN);
    return false;
  }
  lv_obj_set_size(mapDots[index], size, size);
  lv_obj_set_pos(mapDots[index], x, y);
  lv_obj_set_style_bg_color(mapDots[index], color, 0);
  lv_obj_set_style_border_width(mapDots[index], 0, 0);
  lv_obj_clear_flag(mapDots[index], LV_OBJ_FLAG_HIDDEN);
  return true;
}

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
  int mapZoom = findBestMapZoom(centerLat, centerLon, tilePath, sizeof(tilePath));
  if (mapZoomedOut) mapZoom = max(MAP_TILE_MIN_ZOOM, mapZoom - 2);
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
  }

  refreshDashboardLabels();

  if (currentPage == pageSystemInterface && lblSystemInterface) {
    String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("off");
    char interfaceText[360];
    snprintf(interfaceText, sizeof(interfaceText),
             "S3 interface\n"
             "Uptime: %lu s\n"
             "WiFi: %s\n\n"
             "Memory\n"
             "Heap free/min: %lu/%lu KB\n"
             "PSRAM free: %lu KB\n\n"
             "SD card\n"
             "Status: %s\n"
             "Type: %s\n"
             "Used/total: %lu/%lu MB\n"
             "Writes/errors: %lu/%lu\n\n"
             "UI\n"
             "Frames decoded: %lu",
             (unsigned long)(millis() / 1000),
              wifiIp.c_str(),
             (unsigned long)(ESP.getFreeHeap() / 1024),
             (unsigned long)(ESP.getMinFreeHeap() / 1024),
             (unsigned long)(ESP.getFreePsram() / 1024),
             sdStorage.status,
             sdStorage.cardType,
             bytesToWholeMb(sdStorage.usedBytes),
             bytesToWholeMb(sdStorage.totalBytes),
             (unsigned long)sdStorage.writes,
             (unsigned long)sdStorage.writeErrors,
             (unsigned long)framesDecoded);
    lv_label_set_text(lblSystemInterface, interfaceText);
  }

  if (currentPage == pageSystemSerial && lblSystemSerial) {
    char serialText[340];
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
    refreshNodeList(true);
    clearUiDirty(UI_DIRTY_NODES);
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
               "%s | heard %lu s\n"
               "Signal: %.1f dB / %ld dBm | hops %u\n"
               "Channel: %u %s | port %lu\n"
               "Packets: %lu total\n\n"
               "%s\n\n"
               "%s\n\n"
               "Text/Tel/Pos/Enc: %lu/%lu/%lu/%lu",
               node->name,
               (unsigned long)node->num,
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

  if (currentPage == pageBacklight && lblBacklight) {
    char backlightText[96];
    snprintf(backlightText, sizeof(backlightText),
             "Brightness: %u%%\nPWM: %s",
             backlightPercent,
             backlightPwmReady ? "LEDC" : "analog fallback");
    lv_label_set_text(lblBacklight, backlightText);
  }

  if (currentPage == pageBattery && lblBatteryStats) {
    char batteryText[720];
    uint32_t gaugeAge = localBattery.lastGaugeMs ? (millis() - localBattery.lastGaugeMs) / 1000 : 0;
    if (localBattery.gaugePresent) {
      snprintf(batteryText, sizeof(batteryText),
               "LiPo fuel gauge\n"
               "Chip: MAX17048  v0x%04X\n"
               "Displayed: %d%%\n"
               "Displayed source: %s\n"
               "Gauge SOC: %.1f%%  raw 0x%04X\n"
               "MAX corrected: %d%%  trim %.1f%%\n"
               "Pack: %.3f V\n"
               "Code voltage estimate: %d%%\n"
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
               localBattery.percentSource,
               localBattery.gaugeSoc,
               localBattery.rawGaugeSoc,
               localBattery.correctedGaugePercent,
               localBattery.calibrationOffsetTenths / 10.0f,
               localBattery.batteryMv / 1000.0f,
               localBattery.voltagePercent,
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
    "[LINK] rx=%lu tx=%lu last=%s frames=%lu decoded=%lu err=%lu magic=%lu/%lu badlen=%lu ascii=\"%.60s\"\n",
    (unsigned long)bytesFromRadio,
    (unsigned long)bytesToRadio,
    lastByteMs ? String(age).c_str() : "never",
    (unsigned long)streamFrames,
    (unsigned long)framesDecoded,
    (unsigned long)decodeErrors,
    (unsigned long)magic1Count,
    (unsigned long)magic2Count,
    (unsigned long)invalidFrameLengths,
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
      "[SD] available=%s status=\"%s\" type=%s card=%llu total=%llu used=%llu writes=%lu errors=%lu\n",
      sdStorage.available ? "true" : "false",
      sdStorage.status,
      sdStorage.cardType,
      (unsigned long long)sdStorage.cardSizeBytes,
      (unsigned long long)sdStorage.totalBytes,
      (unsigned long long)sdStorage.usedBytes,
      (unsigned long)sdStorage.writes,
      (unsigned long)sdStorage.writeErrors
    );
    Serial.printf(
      "[TOUCH] samples=%lu last=%s x=%u y=%u age=%lu\n",
      (unsigned long)touchSamples,
      lastTouchMs ? "yes" : "no",
      (unsigned)lastTouchX,
      (unsigned)lastTouchY,
      lastTouchMs ? (unsigned long)((millis() - lastTouchMs) / 1000) : 0
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
        Serial.printf("[sd] available=%s status=%s type=%s total=%llu used=%llu tileRoot=%s tileRootFound=%s chat=%u/%u/%u\n",
                      sdStorage.available ? "true" : "false",
                      sdStorage.status,
                      sdStorage.cardType,
                      (unsigned long long)sdStorage.totalBytes,
                      (unsigned long long)sdStorage.usedBytes,
                      mapTileRoot,
                      mapTileRootFound ? "true" : "false",
                      (unsigned)strlen(publicChatLog),
                      (unsigned)strlen(familyChatLog),
                      (unsigned)strlen(directChatLog));
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
    if (channels[i].enabled && channels[i].index == (int8_t)index && channels[i].name[0]) return channels[i].name;
  }
  static char fallback[16];
  if (index == PUBLIC_CHANNEL_INDEX) return "primary";
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
    lv_label_set_text(lblPublicChannelStats, text);
  }
  if (lblPrivateChannelStats) {
    uint8_t privateIndex = privateChannelIndex >= 0 ? (uint8_t)privateChannelIndex : 1;
    snprintf(text, sizeof(text), "Ch %u: %s", (unsigned)privateIndex, privateName);
    lv_label_set_text(lblPrivateChannelStats, text);
  }
  if (lblPrivateChannelHint) {
    uint8_t privateIndex = privateChannelIndex >= 0 ? (uint8_t)privateChannelIndex : 1;
    snprintf(text, sizeof(text), "Ch %u on radio: %s", (unsigned)privateIndex, privateName);
    lv_label_set_text(lblPrivateChannelHint, text);
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
      appendPacketEvent("[config] position config cached\n");
      break;
    case meshtastic_Config_power_tag:
      heltecConfig.power = config.payload_variant.power;
      heltecConfig.hasPower = true;
      appendPacketEvent("[config] power config cached\n");
      break;
    default:
      appendPacketEvent("[config] unsupported config cached counter only\n");
      break;
  }
}

static void cacheHeltecModuleConfig(const meshtastic_ModuleConfig& moduleConfig) {
  heltecConfig.lastModuleMs = millis();
  if (moduleConfig.which_payload_variant == meshtastic_ModuleConfig_serial_tag) {
    heltecConfig.serial = moduleConfig.payload_variant.serial;
    heltecConfig.hasSerial = true;
    appendPacketEvent("[config] serial module cached\n");
  } else {
    appendPacketEvent("[config] unsupported module cached counter only\n");
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
    updated |= setReadonlyTextStable(taPublicChat, publicChatLog[0] ? publicChatLog : emptyText);
  }
  if (currentPage == pagePrivateChat && taFamilyChat) {
    formatNoChannelChat(privateChannelLabel(), emptyText, sizeof(emptyText));
    updated |= setReadonlyTextStable(taFamilyChat, familyChatLog[0] ? familyChatLog : emptyText);
  }
  if (currentPage == pageDirectChat && taDirectChat) {
    updated |= setReadonlyTextStable(taDirectChat, directChatLog[0] ? directChatLog : "No direct messages yet");
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

static void recordTxMessage(uint8_t channel, uint32_t to, const char* text, size_t len, bool ok) {
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

static const char* txStatusText(const TxRecord& record, uint32_t nowMs) {
  if (!record.ok) return "failed";
  if (record.echoSeen) return "echo";
  if (nowMs - record.sentMs > 15000) return "no echo";
  return "sent";
}

static void refreshTxHistoryView() {
  if (!taTxHistory || currentPage != pageTxHistory) return;
  if (txHistoryCount == 0) {
    setReadonlyTextStable(taTxHistory, "No messages sent yet");
    clearUiDirty(UI_DIRTY_TX);
    return;
  }
  uint32_t now = millis();
  char text[1200];
  size_t used = 0;
  for (size_t offset = 0; offset < txHistoryCount && used < sizeof(text) - 1; offset++) {
    size_t index = (txHistoryNext + TX_HISTORY_COUNT - 1 - offset) % TX_HISTORY_COUNT;
    TxRecord& record = txHistory[index];
    char dest[24];
    describeTxDestination(record, dest, sizeof(dest));
    int written = snprintf(text + used,
                           sizeof(text) - used,
                           "%lus  %s  ch%u %s\n%.63s\n\n",
                           (unsigned long)((now - record.sentMs) / 1000),
                           txStatusText(record, now),
                           record.channel,
                           dest,
                           record.text);
    if (written <= 0) break;
    used += min((size_t)written, sizeof(text) - used - 1);
  }
  text[used] = '\0';
  setReadonlyTextStable(taTxHistory, text);
  clearUiDirty(UI_DIRTY_TX);
}

static bool retryLastTx() {
  TxRecord* record = latestTxRecord();
  if (!record || !record->text[0]) {
    appendLine(eventLog, LOG_SIZE, "[local] retry failed: no TX history\n");
    refreshTxHistoryView();
    return false;
  }
  bool ok = record->direct ? sendDirectTextMessage(record->text, record->to) : sendTextMessage(record->text, record->channel);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] retried last TX\n" : "[local] retry failed\n");
  return ok;
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

static void writeStreamFrame(const uint8_t* payload, size_t len) {
  SerialLoRa.write(0x94);
  SerialLoRa.write(0xC3);
  SerialLoRa.write((len >> 8) & 0xFF);
  SerialLoRa.write(len & 0xFF);
  SerialLoRa.write(payload, len);
  bytesToRadio += len + 4;
}

static bool sendConfigRequest() {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_want_config_id_tag;
  toRadio.want_config_id = 1;

  uint8_t out[64];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  writeStreamFrame(out, stream.bytes_written);
  lastConfigRequestMs = millis();
  if (configRequestCount < 255) configRequestCount++;
  appendLine(eventLog, LOG_SIZE, "[local] requested Heltec config\n");
  return true;
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
  if (!text || !text[0]) return false;

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = BROADCAST_ADDR;
  packet.channel = channelIndex < 0 ? PUBLIC_CHANNEL_INDEX : channelIndex;
  packet.want_ack = false;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false);
    return false;
  }
  writeStreamFrame(out, stream.bytes_written);

  rememberLocalSentText(packet.channel, packet.to, text, data.payload.size);
  recordTxMessage(packet.channel, packet.to, text, data.payload.size, true);
  appendChatMessage(packet.channel, packet.to, "me", data.payload.bytes, data.payload.size);

  char line[96];
  snprintf(line, sizeof(line), "[local] text sent on %s\n", channelName(packet.channel));
  appendLine(eventLog, LOG_SIZE, line);
  return true;
}

static bool sendDirectTextMessage(const char* text, uint32_t toNode) {
  if (!text || !text[0] || !isDirectAddress(toNode)) return false;

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = toNode;
  packet.channel = PUBLIC_CHANNEL_INDEX;
  packet.want_ack = true;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) {
    recordTxMessage(packet.channel, packet.to, text, data.payload.size, false);
    return false;
  }
  writeStreamFrame(out, stream.bytes_written);

  rememberLocalSentText(packet.channel, packet.to, text, data.payload.size);
  recordTxMessage(packet.channel, packet.to, text, data.payload.size, true);
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
  writeStreamFrame(out, stream.bytes_written);
  return true;
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

static bool sendHeltecSerialConfig(bool enabled, uint32_t rxd, uint32_t txd, const String& baud, const String& mode, bool echo, bool overrideConsole) {
  meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_zero;
  admin.which_payload_variant = meshtastic_AdminMessage_set_module_config_tag;
  admin.set_module_config.which_payload_variant = meshtastic_ModuleConfig_serial_tag;
  admin.set_module_config.payload_variant.serial.enabled = enabled;
  admin.set_module_config.payload_variant.serial.echo = echo;
  admin.set_module_config.payload_variant.serial.rxd = rxd;
  admin.set_module_config.payload_variant.serial.txd = txd;
  admin.set_module_config.payload_variant.serial.baud = parseSerialBaud(baud);
  admin.set_module_config.payload_variant.serial.mode = parseSerialMode(mode);
  admin.set_module_config.payload_variant.serial.override_console_serial_port = overrideConsole;
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
  admin.set_config.payload_variant.device.role = parseDeviceRole(role);
  admin.set_config.payload_variant.device.rebroadcast_mode = parseRebroadcastMode(rebroadcast);
  admin.set_config.payload_variant.device.node_info_broadcast_secs = nodeInfoSecs;
  admin.set_config.payload_variant.device.led_heartbeat_disabled = ledOff;
  admin.set_config.payload_variant.device.buzzer_mode = parseBuzzerMode(buzzer);
  if (tz.length()) strlcpy(admin.set_config.payload_variant.device.tzdef, tz.c_str(), sizeof(admin.set_config.payload_variant.device.tzdef));
  bool ok = sendLocalAdmin(admin);
  appendLine(eventLog, LOG_SIZE, ok ? "[local] Heltec device config sent\n" : "[local] Heltec device config failed\n");
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

  if (data.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP ||
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
  if (fromRadio.which_payload_variant == meshtastic_FromRadio_packet_tag) {
    handleDecodedPacket(fromRadio.packet);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_my_info_tag) {
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
    }
  } else {
    otherFrames++;
    appendPacketEvent("[radio] other FromRadio frame\n");
  }
}

static void pollLoRa() {
  enum State { MAGIC_1, MAGIC_2, LEN_1, LEN_2, PAYLOAD };
  static State state = MAGIC_1;
  static uint8_t frame[FRAME_MAX];
  static uint16_t frameLen = 0;
  static uint16_t framePos = 0;

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

    switch (state) {
      case MAGIC_1:
        state = (c == 0x94) ? MAGIC_2 : MAGIC_1;
        break;
      case MAGIC_2:
        if (c == 0xC3) {
          magic2Count++;
          state = LEN_1;
        } else {
          state = MAGIC_1;
        }
        break;
      case LEN_1:
        frameLen = ((uint16_t)c) << 8;
        state = LEN_2;
        break;
      case LEN_2:
        frameLen |= c;
        framePos = 0;
        if (frameLen > 0 && frameLen <= FRAME_MAX) {
          streamFrames++;
          state = PAYLOAD;
        } else {
          invalidFrameLengths++;
          state = MAGIC_1;
        }
        break;
      case PAYLOAD:
        frame[framePos++] = c;
        if (framePos >= frameLen) {
          decodeFromRadio(frame, frameLen);
          state = MAGIC_1;
        }
        break;
    }
  }
}

static String jsonEscape(const char* text) {
  String out;
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

static bool requireWebAuth() {
  if (server.authenticate(webUiUser, webUiPass)) {
    if (usingDefaultWebCredentials() && !setupRouteAllowed()) {
      server.send(403, "text/plain", "setup required");
      return false;
    }
    return true;
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
  String json = "{";
  json += "\"title\":\"" + jsonEscape(interfaceDeviceName) + "\",";
  json += "\"deviceName\":\"" + jsonEscape(interfaceDeviceName) + "\",";
  json += "\"hostname\":\"" + jsonEscape(interfaceHostname) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(interfaceApSsid) + "\",";
  json += "\"apChannel\":" + String(interfaceApChannel) + ",";
  json += "\"webUser\":\"" + jsonEscape(webUiUser) + "\",";
  json += "\"setupRequired\":" + String(usingDefaultWebCredentials() ? "true" : "false") + ",";
  json += "\"firmwareVersion\":\"" + String(FIRMWARE_VERSION) + "\",";
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
  json += "\"frames\":" + String(framesDecoded) + ",";
  json += "\"errors\":" + String(decodeErrors) + ",";
  json += "\"bytes\":" + String(bytesFromRadio) + ",";
  json += "\"txBytes\":" + String(bytesToRadio) + ",";
  json += "\"lastByte\":\"" + jsonEscape(rxAge) + "\",";
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
  json += "\"myNode\":\"!" + String(stats.myNodeNum, HEX) + "\",";
  json += "\"myNodeName\":\"" + jsonEscape(nodeName(stats.myNodeNum)) + "\",";
  json += "\"battery\":" + String(localBattery.percent) + ",";
  json += "\"voltage\":" + String(localBattery.batteryMv / 1000.0f, 2) + ",";
  json += "\"fuelGauge\":\"" + String(localBattery.gaugePresent ? "MAX17048" : "none") + "\",";
  json += "\"fuelGaugePresent\":" + String(localBattery.gaugePresent ? "true" : "false") + ",";
  json += "\"fuelGaugeSoc\":" + String(localBattery.gaugeSoc, 1) + ",";
  json += "\"fuelGaugeCorrectedSoc\":" + String(localBattery.correctedGaugeSoc, 1) + ",";
  json += "\"fuelGaugeCorrectedPercent\":" + String(localBattery.correctedGaugePercent) + ",";
  json += "\"fuelGaugeSocReliable\":" + String(localBattery.gaugeSocReliable ? "true" : "false") + ",";
  json += "\"fuelGaugeRawSoc\":\"0x" + String(localBattery.rawGaugeSoc, HEX) + "\",";
  json += "\"fuelGaugeQuickStartSent\":" + String(localBattery.quickStartSent ? "true" : "false") + ",";
  json += "\"voltageBatteryEstimate\":" + String(localBattery.voltagePercent) + ",";
  json += "\"fuelGaugeVersion\":\"0x" + String(localBattery.gaugeVersion, HEX) + "\",";
  json += "\"senseVoltage\":" + String(localBattery.filteredPackMv / 1000.0f, 2) + ",";
  json += "\"rawSenseVoltage\":" + String(localBattery.rawPackMv / 1000.0f, 2) + ",";
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
  json += "\"batteryTrendWindowSec\":" + String(trendWindowSec) + ",";
  json += "\"batteryCalibrationOffsetPercent\":" + String(localBattery.calibrationOffsetTenths / 10.0f, 1) + ",";
  json += "\"batteryCalibrationOffsetMv\":" + String(localBattery.calibrationOffsetMv) + ",";
  json += "\"batteryLearnedVoltage\":" + String(localBattery.learnedBatteryMv / 1000.0f, 2) + ",";
  json += "\"batteryStableSamples\":" + String(localBattery.stableSampleCount) + ",";
  json += "\"sdAvailable\":" + String(sdStorage.available ? "true" : "false") + ",";
  json += "\"sdStatus\":\"" + jsonEscape(sdStorage.status) + "\",";
  json += "\"sdType\":\"" + jsonEscape(sdStorage.cardType) + "\",";
  json += "\"sdUsedKb\":" + String(bytesToWholeKb(sdStorage.usedBytes)) + ",";
  json += "\"sdTotalKb\":" + String(bytesToWholeKb(sdStorage.totalBytes)) + ",";
  json += "\"sdUsedMb\":" + String(bytesToWholeMb(sdStorage.usedBytes)) + ",";
  json += "\"sdSizeMb\":" + String(bytesToWholeMb(sdStorage.totalBytes)) + ",";
  json += "\"sdWrites\":" + String(sdStorage.writes) + ",";
  json += "\"sdErrors\":" + String(sdStorage.writeErrors) + ",";
  json += "\"rx\":" + String(stats.packetsRx) + ",";
  json += "\"tx\":" + String(stats.packetsTx) + ",";
  json += "\"online\":" + String(stats.onlineNodes) + ",";
  json += "\"total\":" + String(stats.totalNodes) + ",";
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
  json += "\"gpsValid\":" + String(gpsStats.valid ? "true" : "false") + ",";
  json += "\"gpsLat\":" + String(gpsStats.latitude, 6) + ",";
  json += "\"gpsLon\":" + String(gpsStats.longitude, 6) + ",";
  json += "\"localGpsBytes\":" + String(gpsBytesFromLocal) + ",";
  json += "\"localGpsSentences\":" + String(localGps.sentencesWithFix()) + ",";
  json += "\"localGpsFailedChecksum\":" + String(localGps.failedChecksum()) + ",";
  json += "\"positionedNodes\":" + String(countPositionedNodes()) + ",";
  json += "\"mapCacheStatus\":\"" + jsonEscape(mapCacheStatus) + "\",";
  json += "\"mapCacheLoaded\":" + String(mapCanvasCached ? "true" : "false") + ",";
  json += "\"mapTileRoot\":\"" + jsonEscape(mapTileRoot) + "\",";
  json += "\"mapTileRootFound\":" + String(mapTileRootFound ? "true" : "false") + ",";
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
  json += "\"wake\":" + String(heltecConfig.power.min_wake_secs) + "}},";
  json += "\"log\":\"" + jsonEscape(eventLog) + "\",";
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
    json += "\"positionAge\":" + String(nodes[i].lastPositionMs ? (millis() - nodes[i].lastPositionMs) / 1000 : 0) + "}";
  }
  json += "]}";
  return json;
}

static void handleStatus() {
  if (!requireWebAuth()) return;
  server.send(200, "application/json", buildStatusJson());
}

static void handleStatusSnapshot() {
  if (!requireWebAuth()) return;
  String json = buildStatusJson();
  if (!sdStorage.available) {
    server.send(503, "text/plain", "SD card not available");
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
  bool ok = file.print(json);
  file.close();
  if (ok) {
    sdStorage.writes++;
    strlcpy(sdStorage.status, "snapshot saved", sizeof(sdStorage.status));
    appendLine(eventLog, LOG_SIZE, "[sd] status snapshot saved\n");
    server.send(200, "application/json", json);
  } else {
    sdStorage.writeErrors++;
    strlcpy(sdStorage.status, "snapshot write failed", sizeof(sdStorage.status));
    server.send(500, "text/plain", "snapshot write failed");
  }
}

static void handleSdDownload(const char* path, const char* downloadName, const char* contentType) {
  if (!requireWebAuth()) return;
  if (!sdStorage.available) {
    server.send(503, "text/plain", "SD card not available");
    return;
  }
  if (!SD_MMC.exists(path)) {
    server.send(404, "text/plain", "file not found");
    return;
  }
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) {
    server.send(500, "text/plain", "open failed");
    return;
  }
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + downloadName + "\"");
  server.streamFile(file, contentType);
  file.close();
}

static void handleSend() {
  if (!requireWebAuth()) return;
  if (!server.hasArg("msg")) {
    server.send(400, "text/plain", "missing msg");
    return;
  }
  String channel = server.hasArg("channel") ? server.arg("channel") : String("public");
  int8_t channelIndex = PUBLIC_CHANNEL_INDEX;
  if (channel.startsWith("ch:")) {
    int parsed = channel.substring(3).toInt();
    if (parsed >= 0 && parsed < (int)MAX_CHANNELS) channelIndex = (int8_t)parsed;
  } else if (channel.length() == 1 && isDigit(channel[0])) {
    int parsed = channel.toInt();
    if (parsed >= 0 && parsed < (int)MAX_CHANNELS) channelIndex = (int8_t)parsed;
  } else if (channel == "secondary" || channel == "family" || channel == "private") {
    channelIndex = privateChannelIndex >= 0 ? privateChannelIndex : 1;
  }
  bool ok = sendTextMessage(server.arg("msg").c_str(), channelIndex);
  server.send(ok ? 200 : 500, "text/plain", ok ? "sent" : "send failed");
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

static void handleRoot() {
  if (!requireWebAuth()) return;
  server.send_P(200, "text/html", WEB_UI_HTML);
}

static void handleFirmwareUpload() {
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

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("[boot] serial ready");
  allocateRuntimeBuffers();
  SerialLoRa.setRxBufferSize(4096);
  SerialLoRa.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("[boot] prefs");
  prefs.begin("s3-lora", false);
  wifiApMode = prefs.getBool("wifiApMode", true);
  readableTextMode = prefs.getBool("readableText", false);
#if ONE_TIME_CHANNEL_PROVISION
  channelProvisionDone = prefs.getBool(CHANNEL_PROVISION_PREF_KEY, false);
  channelProvisionStep = 0;
  nextChannelProvisionMs = millis() + CHANNEL_PROVISION_BOOT_DELAY_MS;
#endif
  localBattery.calibrationOffsetTenths = prefs.getShort("batTrim10", 0);
  localBattery.calibrationOffsetTenths = constrain(localBattery.calibrationOffsetTenths, -120, 120);
  localBattery.calibrationOffsetMv = 0;
  loadInterfaceSettings();
  applyFontMode();
  Serial.println("[boot] init screen");
  initScreen();
  Serial.println("[boot] init sd");
  initSdStorage();

  char bootLine[96];
  snprintf(bootLine, sizeof(bootLine), "[boot] %s starting\n", interfaceDeviceName);
  appendLine(eventLog, LOG_SIZE, bootLine);
#if ONE_TIME_CHANNEL_PROVISION
  appendLine(eventLog, LOG_SIZE, channelProvisionDone ? "[provision] already completed\n" : "[provision] queued for Public/Family channel setup\n");
#endif
  copyPreferenceString("wifiSsid", wifiLocalSsid, sizeof(wifiLocalSsid), "");
  copyPreferenceString("wifiPass", wifiLocalPass, sizeof(wifiLocalPass), "");
  if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  if (taWifiPass) lv_textarea_set_text(taWifiPass, wifiLocalPass);
  if (swWifiApMode) {
    if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiApMode, LV_STATE_CHECKED);
  }

  Serial.println("[boot] routes");
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/serial_cmd", HTTP_POST, handleSerialCmd);
  server.on("/firmware", HTTP_POST, handleFirmwareUpdateDone, handleFirmwareUpload);
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
                          (uint8_t)server.arg("apChannel").toInt(),
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
    initSdStorage();
    server.send(sdStorage.available ? 200 : 503, "text/plain", sdStorage.status);
  });
  server.on("/nodes/clear", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    clearNodeList();
    server.send(200, "text/plain", "node list reset");
  });
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
    bool ok = sendHeltecLoraConfig(server.arg("region"), server.arg("preset"), (uint8_t)server.arg("hop").toInt(), (int8_t)server.arg("tx").toInt());
    server.send(ok ? 200 : 500, "text/plain", ok ? "lora config sent" : "lora config failed");
  });
  server.on("/heltec/serial", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    uint32_t rxd = server.hasArg("rxd") ? (uint32_t)server.arg("rxd").toInt() : 38;
    uint32_t txd = server.hasArg("txd") ? (uint32_t)server.arg("txd").toInt() : 39;
    String baud = server.arg("baud");
    String mode = server.arg("mode");
    bool ok = sendHeltecSerialConfig(server.arg("enabled") == "1",
                                     rxd,
                                     txd,
                                     baud.length() ? baud : String("115200"),
                                     mode.length() ? mode : String("PROTO"),
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
                                     (uint32_t)server.arg("nodeInfo").toInt(),
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
                                       (uint32_t)server.arg("broadcast").toInt(),
                                       (uint32_t)server.arg("gpsUpdate").toInt(),
                                       (uint32_t)server.arg("gpsAttempt").toInt(),
                                       (uint32_t)server.arg("smartMeters").toInt(),
                                       (uint32_t)server.arg("smartSecs").toInt());
    server.send(ok ? 200 : 500, "text/plain", ok ? "position config sent" : "position config failed");
  });
  server.on("/heltec/power", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    bool ok = sendHeltecPowerConfig(server.arg("saving") == "1",
                                    (uint32_t)server.arg("shutdown").toInt(),
                                    (uint32_t)server.arg("waitBt").toInt(),
                                    (uint32_t)server.arg("sds").toInt(),
                                    (uint32_t)server.arg("ls").toInt(),
                                    (uint32_t)server.arg("wake").toInt());
    server.send(ok ? 200 : 500, "text/plain", ok ? "power config sent" : "power config failed");
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
    bool ok = sendHeltecChannelConfig((uint8_t)server.arg("index").toInt(),
                                      server.arg("role"),
                                      server.arg("name"),
                                      server.arg("psk"),
                                      server.arg("uplink") == "1",
                                      server.arg("downlink") == "1",
                                      server.hasArg("positionPrecision") ? server.arg("positionPrecision").toInt() : -1);
    server.send(ok ? 200 : 500, "text/plain", ok ? "channel sent" : "channel failed");
  });
  Serial.println("[boot] wifi");
  startWifi();
  Serial.println("[boot] wifi done");

  char line[128];
  String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("0.0.0.0");
  snprintf(line, sizeof(line), "[boot] WiFi %s %s at %s\n",
           wifiEnabled ? "on" : "off",
           wifiApMode ? "AP" : "Local",
           wifiIp.c_str());
  appendLine(eventLog, LOG_SIZE, line);
  Serial.println("[boot] config request");
  sendConfigRequest();
  Serial.println("[boot] setup complete");
}

void loop() {
  pollLocalGps();
  serviceUsbSerialCommands();
  pollLoRa();
  serviceConfigRequests();
  serviceOneTimeChannelProvision();
  if (wifiEnabled) server.handleClient();
  if (otaRestartPending && (int32_t)(millis() - otaRestartAtMs) >= 0) {
    Serial.println("[ota] restarting");
    delay(100);
    ESP.restart();
  }
  serviceScreen();
  printSerialDiagnostics();
  delay(2);
}

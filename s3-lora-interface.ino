#include <Arduino.h>
#include <limits.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <TinyGPSPlus.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "constants.h"
#include "src/meshtastic/mesh.pb.h"
#include "src/meshtastic/telemetry.pb.h"
#include "src/meshtastic/portnums.pb.h"
#include "src/UIConfig.h"

HardwareSerial SerialLoRa(2);
HardwareSerial SerialGPS(1);
WebServer server(80);
TFT_eSPI tft;
TinyGPSPlus localGps;

struct NodeRecord {
  uint32_t num = 0;
  char name[40] = "";
  float snr = 0.0f;
  uint32_t lastHeardMs = 0;
  bool hasPosition = false;
  double latitude = 0.0;
  double longitude = 0.0;
  int32_t altitude = 0;
  uint32_t lastPositionMs = 0;
};

struct ChannelRecord {
  int8_t index = -1;
  char name[12] = "";
  bool enabled = false;
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
  uint32_t rawMv = 0;
  uint32_t rawPackMv = 0;
  uint32_t filteredPackMv = 0;
  uint32_t batteryMv = 0;
  uint32_t lastRawPackMv = 0;
  int32_t deltaMvPerMin = 0;
  int percent = 0;
  bool usbLikely = false;
  bool chargingLikely = false;
  uint32_t lastSampleMs = 0;
  uint32_t lastTrendMs = 0;
  uint32_t lastTrendMv = 0;
  char powerState[24] = "unknown";
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
static constexpr size_t MAX_NODES = 64;
static constexpr size_t MAP_DOT_COUNT = MAX_NODES + 1;
static constexpr size_t MAX_CHANNELS = 8;
static constexpr size_t FRAME_MAX = 512;
static constexpr int STATUS_BAR_H = 20;
static constexpr int NAV_BAR_H = 40;
static constexpr int MAP_PLOT_W = SCREEN_W - 16;
static constexpr int MAP_PLOT_H = 136;
static constexpr int MAP_TILE_SIZE = 256;
static constexpr int MAP_TILE_MIN_ZOOM = 10;
static constexpr int MAP_TILE_MAX_ZOOM = 14;
static constexpr uint32_t MAP_CACHE_MAGIC = 0x4D415031UL;
static constexpr uint16_t MAP_CACHE_VERSION = 1;
static constexpr int8_t PUBLIC_CHANNEL_INDEX = 0;
static const char* SD_DIR = "/s3-lora";
static const char* SD_EVENTS_PATH = "/s3-lora/events.log";
static const char* SD_PUBLIC_CHAT_PATH = "/s3-lora/public_chat.log";
static const char* SD_PRIVATE_CHAT_PATH = "/s3-lora/private_chat.log";
static const char* SD_POSITIONS_PATH = "/s3-lora/positions.csv";
static const char* SD_MAP_CACHE_PATH = "/s3-lora/map_cache.bin";
static const char* SD_LAST_LOCATION_PATH = "/s3-lora/last_location.txt";
static constexpr uint32_t COLOR_BG = 0x050807;
static constexpr uint32_t COLOR_PANEL = 0x101816;
static constexpr uint32_t COLOR_INPUT = 0x07100D;
static constexpr uint32_t COLOR_BORDER = 0x24483E;
static constexpr uint32_t COLOR_TEXT = 0xF4FFF9;
static constexpr uint32_t COLOR_MUTED = 0x8AB7A6;
static constexpr uint32_t COLOR_ACCENT = 0x68FFC0;
static constexpr uint32_t COLOR_ACTION = 0x00C985;

static char eventLog[LOG_SIZE];
static char publicChatLog[CHAT_SIZE];
static char privateChatLog[CHAT_SIZE];
static char lastLocalSentText[234];
static NodeRecord nodes[MAX_NODES];
static ChannelRecord channels[MAX_CHANNELS];
static size_t nodeCount = 0;
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
static uint32_t wifiStartedMs = 0;
static uint32_t wifiStoppedMs = 0;
static uint32_t wifiToggleCount = 0;
static uint8_t backlightPercent = 80;
static bool backlightPwmReady = false;
static uint32_t lastLocalSentMs = 0;
static uint8_t lastLocalSentChannel = PUBLIC_CHANNEL_INDEX;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_disp_t* display = nullptr;
static lv_color_t lvBuf1[SCREEN_W * 24];
static lv_color_t lvBuf2[SCREEN_W * 24];
static lv_color_t mapCanvasBuf[MAP_PLOT_W * MAP_PLOT_H];
static uint16_t mapReadBuf[MAP_PLOT_W];
static lv_obj_t* pageLauncher = nullptr;
static lv_obj_t* pageLora = nullptr;
static lv_obj_t* pagePublicChat = nullptr;
static lv_obj_t* pagePrivateChat = nullptr;
static lv_obj_t* pageGps = nullptr;
static lv_obj_t* pageSystem = nullptr;
static lv_obj_t* pageSystemInterface = nullptr;
static lv_obj_t* pageSystemSerial = nullptr;
static lv_obj_t* pageSystemRadio = nullptr;
static lv_obj_t* pageSystemGps = nullptr;
static lv_obj_t* pageWifi = nullptr;
static lv_obj_t* pageWifiStats = nullptr;
static lv_obj_t* pageBacklight = nullptr;
static lv_obj_t* pageBattery = nullptr;
static lv_obj_t* currentPage = nullptr;
static lv_obj_t* previousPage = nullptr;
static lv_obj_t* statusBar = nullptr;
static lv_obj_t* navBar = nullptr;
static lv_obj_t* lblStatus = nullptr;
static lv_obj_t* lblBatteryStatus = nullptr;
static lv_obj_t* lblStats = nullptr;
static lv_obj_t* lblSystemInterface = nullptr;
static lv_obj_t* lblSystemSerial = nullptr;
static lv_obj_t* lblSystemRadio = nullptr;
static lv_obj_t* lblWifiState = nullptr;
static lv_obj_t* lblWifiStats = nullptr;
static lv_obj_t* swWifiEnabled = nullptr;
static lv_obj_t* sliderBacklight = nullptr;
static lv_obj_t* lblBacklight = nullptr;
static lv_obj_t* lblBatteryStats = nullptr;
static lv_obj_t* lblGpsStats = nullptr;
static lv_obj_t* lblMapStats = nullptr;
static lv_obj_t* mapPlot = nullptr;
static lv_obj_t* mapCanvas = nullptr;
static lv_obj_t* mapDots[MAP_DOT_COUNT];
static lv_obj_t* taPublicChat = nullptr;
static lv_obj_t* taPrivateChat = nullptr;
static lv_obj_t* taScreenLog = nullptr;
static lv_obj_t* taScreenNodes = nullptr;
static lv_obj_t* taPublicInput = nullptr;
static lv_obj_t* taPrivateInput = nullptr;
static lv_obj_t* activeChatInput = nullptr;
static lv_obj_t* keyboard = nullptr;
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* keyboardScreen = nullptr;
static lv_obj_t* keyboardPrompt = nullptr;
static lv_obj_t* keyboardText = nullptr;
static lv_obj_t* landscapeKeyboard = nullptr;
static bool landscapeKeyboardOpen = false;
static uint32_t lastUiRefreshMs = 0;
static uint32_t lastSerialDiagMs = 0;
static uint32_t lastSdDiagMs = 0;
static uint32_t lastMapUiRefreshMs = 0;
static bool mapCanvasCached = false;
static bool mapRenderPending = false;
static int cachedMapZoom = -1;
static long cachedMapTileX = LONG_MIN;
static long cachedMapTileY = LONG_MIN;
static int cachedMapPixelX = INT_MIN;
static int cachedMapPixelY = INT_MIN;
static bool cachedMapTileFound = false;
static double cachedMapLat = 0.0;
static double cachedMapLon = 0.0;
static char mapCacheStatus[48] = "not checked";
static uint32_t touchSamples = 0;
static uint32_t lastTouchMs = 0;
static uint16_t lastTouchX = 0;
static uint16_t lastTouchY = 0;

static bool sendTextMessage(const char* text, int8_t channelIndex = PUBLIC_CHANNEL_INDEX);
static const char* nodeName(uint32_t num);
static NodeRecord* findOrCreateNode(uint32_t num);
static void appendLine(char* buffer, size_t bufferSize, const char* line);
static void refreshMapUi();
static void loadMapCacheFromSd();
static void refreshChatViews();

static void loadSdTextTail(const char* path, char* buffer, size_t bufferSize) {
  if (!sdStorage.available || !path || !buffer || bufferSize == 0 || !SD_MMC.exists(path)) return;
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) return;
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
}

static void loadChatLogsFromSd() {
  publicChatLog[0] = '\0';
  privateChatLog[0] = '\0';
  loadSdTextTail(SD_PUBLIC_CHAT_PATH, publicChatLog, sizeof(publicChatLog));
  loadSdTextTail(SD_PRIVATE_CHAT_PATH, privateChatLog, sizeof(privateChatLog));
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
  loadChatLogsFromSd();
  loadMapCacheFromSd();
}

static void refreshSdUsage() {
  if (!sdStorage.available) return;
  sdStorage.totalBytes = SD_MMC.totalBytes();
  if (sdStorage.totalBytes == 0) sdStorage.totalBytes = sdStorage.cardSizeBytes;
  sdStorage.usedBytes = SD_MMC.usedBytes();
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
  cachedMapLat = lat;
  cachedMapLon = lon;
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

static int interpolateBatteryPct(uint32_t mv, uint32_t lowMv, uint8_t lowPct, uint32_t highMv, uint8_t highPct) {
  uint32_t mvRange = highMv - lowMv;
  uint32_t pctRange = highPct - lowPct;
  return lowPct + (int)(((mv - lowMv) * pctRange + (mvRange / 2)) / mvRange);
}

static int batteryPercentFromMv(uint32_t mv) {
  struct BatteryCurvePoint {
    uint32_t mv;
    uint8_t pct;
  };

  static const BatteryCurvePoint curve[] = {
    {3300, 0},
    {3500, 10},
    {3600, 25},
    {3650, 35},
    {3680, 42},
    {3700, 46},
    {3750, 55},
    {3800, 62},
    {3850, 72},
    {3920, 80},
    {4000, 88},
    {4100, 95},
    {4200, 100},
  };

  if (mv <= curve[0].mv) return curve[0].pct;
  for (size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); ++i) {
    if (mv <= curve[i].mv) {
      return constrain(interpolateBatteryPct(mv, curve[i - 1].mv, curve[i - 1].pct, curve[i].mv, curve[i].pct), 0, 100);
    }
  }
  return curve[sizeof(curve) / sizeof(curve[0]) - 1].pct;
}

static uint32_t smoothBatteryEstimate(uint32_t currentMv, uint32_t targetMv, bool pluggedIn) {
  if (currentMv == 0) return constrain(targetMv, 3300UL, 4200UL);
  targetMv = constrain(targetMv, 3300UL, 4200UL);

  if (pluggedIn) {
    if (targetMv > currentMv) {
      uint32_t rise = min<uint32_t>(targetMv - currentMv, 2);
      return currentMv + rise;
    }
    return (currentMv * 31 + targetMv) / 32;
  }

  if (targetMv < currentMv) {
    uint32_t fall = min<uint32_t>(currentMv - targetMv, 20);
    return currentMv - fall;
  }
  return (currentMv * 7 + targetMv) / 8;
}

static void sampleLocalBattery() {
  if (millis() - localBattery.lastSampleMs < 1000) return;
  localBattery.lastSampleMs = millis();

  uint32_t adcMv = analogReadMilliVolts(BATT_ADC_PIN);
  uint32_t packMv = (uint32_t)(adcMv * BATT_ADC_MULTIPLIER + 0.5f);
  int32_t rawStepMv = localBattery.lastRawPackMv ? (int32_t)packMv - (int32_t)localBattery.lastRawPackMv : 0;

  localBattery.rawMv = adcMv;
  localBattery.rawPackMv = packMv;
  if (localBattery.filteredPackMv == 0) {
    localBattery.filteredPackMv = packMv;
  } else {
    localBattery.filteredPackMv = (localBattery.filteredPackMv * 3 + packMv) / 4;
  }

  bool overLipoVoltage = localBattery.filteredPackMv >= 4300 || packMv >= 4350;
  bool unplugStep = rawStepMv < -120 && packMv < 4250;
  bool keepUsbLatch = localBattery.usbLikely && localBattery.filteredPackMv >= 4240 && !unplugStep;
  localBattery.usbLikely = overLipoVoltage || keepUsbLatch;

  bool plausibleChargeVoltage = localBattery.filteredPackMv >= 4050 && localBattery.filteredPackMv <= 4240;
  localBattery.chargingLikely = !localBattery.usbLikely && plausibleChargeVoltage && rawStepMv > 8;

  uint32_t trustedPackMv = localBattery.filteredPackMv;
  if (localBattery.usbLikely && trustedPackMv > 4200) {
    trustedPackMv = localBattery.batteryMv ? localBattery.batteryMv : 4100;
  }
  localBattery.batteryMv = smoothBatteryEstimate(localBattery.batteryMv, trustedPackMv, localBattery.usbLikely);
  localBattery.percent = batteryPercentFromMv(localBattery.batteryMv);

  if (localBattery.lastTrendMs == 0) {
    localBattery.lastTrendMs = millis();
    localBattery.lastTrendMv = localBattery.batteryMv;
  } else if (millis() - localBattery.lastTrendMs >= 30000) {
    int32_t delta = (int32_t)localBattery.batteryMv - (int32_t)localBattery.lastTrendMv;
    uint32_t elapsedMs = millis() - localBattery.lastTrendMs;
    localBattery.deltaMvPerMin = (int32_t)((delta * 60000L) / (int32_t)elapsedMs);
    if (localBattery.batteryMv < 4100 && localBattery.deltaMvPerMin > 0) {
      localBattery.deltaMvPerMin = 0;
    }
    localBattery.lastTrendMs = millis();
    localBattery.lastTrendMv = localBattery.batteryMv;
  }

  if (localBattery.usbLikely) {
    strlcpy(localBattery.powerState, "plugged in", sizeof(localBattery.powerState));
  } else if (localBattery.chargingLikely) {
    strlcpy(localBattery.powerState, "charging/plugged", sizeof(localBattery.powerState));
  } else if (localBattery.deltaMvPerMin < -4) {
    strlcpy(localBattery.powerState, "battery only", sizeof(localBattery.powerState));
  } else {
    strlcpy(localBattery.powerState, "battery/steady", sizeof(localBattery.powerState));
  }
  localBattery.lastRawPackMv = packMv;
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
  } else if (buffer == publicChatLog) {
    appendSdLine(SD_PUBLIC_CHAT_PATH, line);
  } else if (buffer == privateChatLog) {
    appendSdLine(SD_PRIVATE_CHAT_PATH, line);
  }
}

static void startWifiAp() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(INTERFACE_AP_SSID, INTERFACE_AP_PASS);
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

static void stopWifiAp() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiEnabled = false;
  wifiStoppedMs = millis();
  appendLine(eventLog, LOG_SIZE, "[wifi] AP disabled\n");
}

static void setWifiEnabled(bool enabled) {
  if (enabled == wifiEnabled) return;
  wifiToggleCount++;
  if (enabled) startWifiAp();
  else stopWifiAp();
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
  pinMode(TFT_BL, OUTPUT);
  backlightPwmReady = ledcAttach(TFT_BL, 5000, 8);
  applyBacklight();
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

static void styleDarkObject(lv_obj_t* obj, uint32_t bg, uint32_t text = COLOR_TEXT) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(bg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_color(obj, lv_color_hex(text), LV_PART_MAIN);
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

static void styleDarkBorder(lv_obj_t* obj, uint32_t color = COLOR_BORDER) {
  lv_obj_set_style_border_color(obj, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
}

static void styleDarkTextArea(lv_obj_t* ta) {
  styleDarkObject(ta, COLOR_INPUT, 0xE8FFF5);
  styleDarkBorder(ta, 0x2F705F);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_MUTED), LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0x16342C), LV_PART_SELECTED);
  lv_obj_set_style_text_color(ta, lv_color_hex(COLOR_TEXT), LV_PART_SELECTED);
}

static lv_obj_t* makePanel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  styleDarkObject(panel, COLOR_PANEL);
  styleDarkBorder(panel);
  lv_obj_set_style_radius(panel, 6, 0);
  lv_obj_set_style_pad_all(panel, 6, 0);
  return panel;
}

static lv_obj_t* makePage(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H - NAV_BAR_H - STATUS_BAR_H);
  lv_obj_align(page, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_H);
  styleDarkObject(page, COLOR_BG);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 6, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  return page;
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
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_style_text_font(lblStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblStatus, LV_ALIGN_LEFT_MID, 0, 0);

  lblBatteryStatus = lv_label_create(statusBar);
  lv_label_set_text(lblBatteryStatus, "Batt --%");
  lv_obj_set_style_text_color(lblBatteryStatus, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_text_font(lblBatteryStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblBatteryStatus, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void showPage(lv_obj_t* target, bool remember = true) {
  if (!target) return;
  if (remember && currentPage && currentPage != target) previousPage = currentPage;
  lv_obj_t* pages[] = {
    pageLauncher, pageLora, pagePublicChat, pagePrivateChat, pageGps,
    pageSystem, pageSystemInterface, pageSystemSerial, pageSystemRadio, pageSystemGps,
    pageWifi, pageWifiStats, pageBacklight, pageBattery
  };
  for (lv_obj_t* page : pages) {
    if (!page) continue;
    if (page == target) lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  currentPage = target;
  if (target == pageGps) {
    mapRenderPending = true;
    lastMapUiRefreshMs = millis();
    if (mapCanvasCached && lblMapStats) {
      char cacheText[180];
      snprintf(cacheText, sizeof(cacheText),
               "Cached map ready\n"
               "Last: %.5f, %.5f  z%d\n"
               "Refreshing shortly...",
               cachedMapLat,
               cachedMapLon,
               cachedMapZoom);
      lv_label_set_text(lblMapStats, cacheText);
    }
  }
}

static lv_obj_t* makeActionButton(lv_obj_t* parent, const char* text, int y, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, SCREEN_W - 18, 44);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
  styleDarkObject(btn, COLOR_PANEL);
  styleDarkBorder(btn, 0x2F705F);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makeSystemTile(lv_obj_t* parent, const char* text, int col, int row, lv_event_cb_t cb) {
  const int tileW = 106;
  const int tileH = 54;
  const int gap = 8;
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
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(label);
  lv_obj_move_foreground(btn);
  return btn;
}

static lv_obj_t* makePageTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* title = lv_label_create(parent);
  lv_label_set_text(title, text);
  lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
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
  lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
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
  makeNavButton(navBar, "Launcher", LV_ALIGN_RIGHT_MID, -4, [](lv_event_t*) {
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
  lv_obj_set_style_text_font(ta, LV_FONT_DEFAULT, 0);
  styleDarkTextArea(ta);
  return ta;
}

static int8_t activeChatChannel() {
  if (activeChatInput == taPrivateInput) {
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
  sendFromInput(activeChatInput, activeChatChannel());
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
  if (send) {
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

static void openLandscapeKeyboard(lv_obj_t* input) {
  if (!input || !keyboardScreen || !keyboardPrompt || !keyboardText || !landscapeKeyboard) return;
  activeChatInput = input;
  lv_label_set_text(keyboardPrompt, input == taPrivateInput ? "Private message" : "Public message");
  lv_textarea_set_text(keyboardText, lv_textarea_get_text(input));
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
    openLandscapeKeyboard((lv_obj_t*)lv_event_get_target(e));
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

static void buildLandscapeKeyboardScreen() {
  keyboardScreen = lv_obj_create(nullptr);
  styleDarkObject(keyboardScreen, COLOR_BG);
  lv_obj_set_style_border_width(keyboardScreen, 0, 0);
  lv_obj_clear_flag(keyboardScreen, LV_OBJ_FLAG_SCROLLABLE);

  keyboardPrompt = lv_label_create(keyboardScreen);
  lv_label_set_text(keyboardPrompt, "Message");
  lv_obj_set_style_text_color(keyboardPrompt, lv_color_hex(COLOR_ACCENT), 0);
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

  buildStatusBar(screen);

  pageLauncher = makePage(screen);
  pageLora = makePage(screen);
  pagePublicChat = makePage(screen);
  pagePrivateChat = makePage(screen);
  pageGps = makePage(screen);
  pageSystem = makePage(screen);
  pageSystemInterface = makePage(screen);
  pageSystemSerial = makePage(screen);
  pageSystemRadio = makePage(screen);
  pageSystemGps = makePage(screen);
  pageWifi = makePage(screen);
  pageWifiStats = makePage(screen);
  pageBacklight = makePage(screen);
  pageBattery = makePage(screen);

  currentPage = pageLauncher;
  lv_obj_clear_flag(pageLauncher, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* launcherTitle = lv_label_create(pageLauncher);
  lv_label_set_text(launcherTitle, "Heltec Interface");
  lv_obj_set_style_text_color(launcherTitle, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_style_text_font(launcherTitle, LV_FONT_DEFAULT, 0);
  lv_obj_align(launcherTitle, LV_ALIGN_TOP_LEFT, 2, 4);

  lv_obj_t* launcherSub = lv_label_create(pageLauncher);
  lv_label_set_text(launcherSub, "Select a module");
  lv_obj_set_style_text_color(launcherSub, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(launcherSub, LV_ALIGN_TOP_LEFT, 2, 24);

  makeSystemTile(pageLauncher, "LoRa", 0, 0, [](lv_event_t*) { showPage(pageLora); });
  makeSystemTile(pageLauncher, "GPS / Map", 1, 0, [](lv_event_t*) { showPage(pageGps); });
  makeSystemTile(pageLauncher, "System", 0, 1, [](lv_event_t*) { showPage(pageSystem); });

  makePageTitle(pageLora, "LoRa");
  makeActionButton(pageLora, "Public Chat", 34, [](lv_event_t*) { showPage(pagePublicChat); });
  makeActionButton(pageLora, "Private Chat", 86, [](lv_event_t*) { showPage(pagePrivateChat); });

  lv_obj_t* loraPanel = makePanel(pageLora);
  lv_obj_set_size(loraPanel, SCREEN_W - 12, 112);
  lv_obj_align(loraPanel, LV_ALIGN_TOP_MID, 0, 140);
  lblStats = lv_label_create(loraPanel);
  lv_label_set_text(lblStats, "Waiting for radio...");
  lv_obj_set_style_text_font(lblStats, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_text_color(lblStats, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_width(lblStats, lv_pct(100));

  makePageTitle(pagePublicChat, "Public Chat");
  lv_obj_t* publicStats = lv_label_create(pagePublicChat);
  lv_label_set_text(publicStats, "Channel: primary");
  lv_obj_set_style_text_color(publicStats, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(publicStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taPublicChat = makeReadonlyText(pagePublicChat, 36, 148);
  lv_textarea_set_text(taPublicChat, "No public chat yet");
  taPublicInput = lv_textarea_create(pagePublicChat);
  lv_obj_set_size(taPublicInput, SCREEN_W - 76, 38);
  lv_obj_align(taPublicInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taPublicInput);
  lv_textarea_set_one_line(taPublicInput, true);
  lv_textarea_set_max_length(taPublicInput, 233);
  lv_textarea_set_placeholder_text(taPublicInput, "Public message");
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

  makePageTitle(pagePrivateChat, "Private Chat");
  lv_obj_t* privateStats = lv_label_create(pagePrivateChat);
  lv_label_set_text(privateStats, "Channel: priv");
  lv_obj_set_style_text_color(privateStats, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(privateStats, LV_ALIGN_TOP_LEFT, 2, 18);

  taPrivateChat = makeReadonlyText(pagePrivateChat, 36, 148);
  lv_textarea_set_text(taPrivateChat, "No private chat yet");
  taPrivateInput = lv_textarea_create(pagePrivateChat);
  lv_obj_set_size(taPrivateInput, SCREEN_W - 76, 38);
  lv_obj_align(taPrivateInput, LV_ALIGN_TOP_LEFT, 6, 192);
  styleDarkTextArea(taPrivateInput);
  lv_textarea_set_one_line(taPrivateInput, true);
  lv_textarea_set_max_length(taPrivateInput, 233);
  lv_textarea_set_placeholder_text(taPrivateInput, "Private message");
  lv_obj_add_event_cb(taPrivateInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* privateSendBtn = lv_btn_create(pagePrivateChat);
  lv_obj_set_size(privateSendBtn, 56, 38);
  lv_obj_align(privateSendBtn, LV_ALIGN_TOP_RIGHT, -6, 192);
  styleDarkObject(privateSendBtn, COLOR_ACTION, 0x001B12);
  lv_obj_set_style_shadow_width(privateSendBtn, 0, 0);
  lv_obj_add_event_cb(privateSendBtn, [](lv_event_t*) { sendFromInput(taPrivateInput, privateChannelIndex >= 0 ? privateChannelIndex : 1); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(privateSendBtn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* privateSendLbl = lv_label_create(privateSendBtn);
  lv_label_set_text(privateSendLbl, "Send");
  lv_obj_set_style_text_color(privateSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(privateSendLbl);
  lv_obj_move_foreground(privateSendBtn);

  lv_obj_t* privHint = lv_label_create(pagePrivateChat);
  lv_label_set_text(privHint, "Meshtastic channel name: priv");
  lv_obj_set_style_text_color(privHint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_align(privHint, LV_ALIGN_TOP_LEFT, 2, 236);

  makePageTitle(pageGps, "GPS / Map");
  mapPlot = makePanel(pageGps);
  lv_obj_set_size(mapPlot, MAP_PLOT_W, MAP_PLOT_H);
  lv_obj_align(mapPlot, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_bg_color(mapPlot, lv_color_hex(COLOR_INPUT), 0);
  lv_obj_set_style_pad_all(mapPlot, 0, 0);
  lv_obj_clear_flag(mapPlot, LV_OBJ_FLAG_SCROLLABLE);
  mapCanvas = lv_canvas_create(mapPlot);
  lv_canvas_set_buffer(mapCanvas, mapCanvasBuf, MAP_PLOT_W, MAP_PLOT_H, LV_IMG_CF_TRUE_COLOR);
  lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
  lv_obj_set_pos(mapCanvas, 0, 0);
  lv_obj_clear_flag(mapCanvas, LV_OBJ_FLAG_CLICKABLE);
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
  lv_obj_align(lblMapStats, LV_ALIGN_TOP_LEFT, 6, 164);

  makePageTitle(pageSystem, "System");
  makeSystemTile(pageSystem, "Interface", 0, 0, [](lv_event_t*) { showPage(pageSystemInterface); });
  makeSystemTile(pageSystem, "Serial", 1, 0, [](lv_event_t*) { showPage(pageSystemSerial); });
  makeSystemTile(pageSystem, "Radio", 0, 1, [](lv_event_t*) { showPage(pageSystemRadio); });
  makeSystemTile(pageSystem, "GPS", 1, 1, [](lv_event_t*) { showPage(pageSystemGps); });
  makeSystemTile(pageSystem, "WiFi", 0, 2, [](lv_event_t*) { showPage(pageWifi); });
  makeSystemTile(pageSystem, "Backlight", 1, 2, [](lv_event_t*) { showPage(pageBacklight); });
  makeSystemTile(pageSystem, "Battery", 0, 3, [](lv_event_t*) { showPage(pageBattery); });

  makePageTitle(pageSystemInterface, "Interface");
  lv_obj_t* interfacePanel = makePanel(pageSystemInterface);
  lv_obj_set_size(interfacePanel, SCREEN_W - 12, 226);
  lv_obj_align(interfacePanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemInterface = lv_label_create(interfacePanel);
  lv_label_set_text(lblSystemInterface, "Interface ready");
  lv_obj_set_style_text_color(lblSystemInterface, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblSystemInterface, lv_pct(100));

  makePageTitle(pageSystemSerial, "Serial Link");
  lv_obj_t* serialPanel = makePanel(pageSystemSerial);
  lv_obj_set_size(serialPanel, SCREEN_W - 12, 226);
  lv_obj_align(serialPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemSerial = lv_label_create(serialPanel);
  lv_label_set_text(lblSystemSerial, "Serial ready");
  lv_obj_set_style_text_color(lblSystemSerial, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblSystemSerial, lv_pct(100));

  makePageTitle(pageSystemRadio, "Radio Stats");
  lv_obj_t* radioPanel = makePanel(pageSystemRadio);
  lv_obj_set_size(radioPanel, SCREEN_W - 12, 226);
  lv_obj_align(radioPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblSystemRadio = lv_label_create(radioPanel);
  lv_label_set_text(lblSystemRadio, "Radio ready");
  lv_obj_set_style_text_color(lblSystemRadio, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblSystemRadio, lv_pct(100));

  makePageTitle(pageSystemGps, "GPS Stats");
  lv_obj_t* gpsPanel = makePanel(pageSystemGps);
  lv_obj_set_size(gpsPanel, SCREEN_W - 12, 226);
  lv_obj_align(gpsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblGpsStats = lv_label_create(gpsPanel);
  lv_label_set_text(lblGpsStats, "Waiting for CYD GPS UART...");
  lv_obj_set_style_text_color(lblGpsStats, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblGpsStats, lv_pct(100));

  makePageTitle(pageWifi, "WiFi");
  lv_obj_t* wifiPanel = makePanel(pageWifi);
  lv_obj_set_size(wifiPanel, SCREEN_W - 12, 112);
  lv_obj_align(wifiPanel, LV_ALIGN_TOP_MID, 0, 26);
  lv_obj_t* wifiToggleLabel = lv_label_create(wifiPanel);
  lv_label_set_text(wifiToggleLabel, "Access point");
  lv_obj_set_style_text_color(wifiToggleLabel, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_align(wifiToggleLabel, LV_ALIGN_TOP_LEFT, 2, 4);
  swWifiEnabled = lv_switch_create(wifiPanel);
  lv_obj_align(swWifiEnabled, LV_ALIGN_TOP_RIGHT, -2, 0);
  if (wifiEnabled) lv_obj_add_state(swWifiEnabled, LV_STATE_CHECKED);
  lv_obj_add_event_cb(swWifiEnabled, [](lv_event_t* e) {
    setWifiEnabled(lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lblWifiState = lv_label_create(wifiPanel);
  lv_label_set_text(lblWifiState, "WiFi starting...");
  lv_obj_set_style_text_color(lblWifiState, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_width(lblWifiState, lv_pct(100));
  lv_obj_align(lblWifiState, LV_ALIGN_TOP_LEFT, 2, 44);
  makeActionButton(pageWifi, "WiFi Stats", 158, [](lv_event_t*) { showPage(pageWifiStats); });

  makePageTitle(pageWifiStats, "WiFi Stats");
  lv_obj_t* wifiStatsPanel = makePanel(pageWifiStats);
  lv_obj_set_size(wifiStatsPanel, SCREEN_W - 12, 226);
  lv_obj_align(wifiStatsPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblWifiStats = lv_label_create(wifiStatsPanel);
  lv_label_set_text(lblWifiStats, "WiFi stats unavailable");
  lv_obj_set_style_text_color(lblWifiStats, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblWifiStats, lv_pct(100));

  makePageTitle(pageBacklight, "Backlight");
  lv_obj_t* backlightPanel = makePanel(pageBacklight);
  lv_obj_set_size(backlightPanel, SCREEN_W - 12, 150);
  lv_obj_align(backlightPanel, LV_ALIGN_TOP_MID, 0, 30);
  lblBacklight = lv_label_create(backlightPanel);
  lv_label_set_text(lblBacklight, "Brightness: 80%");
  lv_obj_set_style_text_color(lblBacklight, lv_color_hex(COLOR_TEXT), 0);
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
  lv_obj_t* backlightHint = lv_label_create(backlightPanel);
  lv_label_set_text(backlightHint, "Lower brightness extends battery runtime.");
  lv_obj_set_style_text_color(backlightHint, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_width(backlightHint, lv_pct(100));
  lv_obj_align(backlightHint, LV_ALIGN_TOP_LEFT, 2, 96);

  makePageTitle(pageBattery, "Battery");
  lv_obj_t* batteryPanel = makePanel(pageBattery);
  lv_obj_set_size(batteryPanel, SCREEN_W - 12, 226);
  lv_obj_align(batteryPanel, LV_ALIGN_TOP_MID, 0, 24);
  lblBatteryStats = lv_label_create(batteryPanel);
  lv_label_set_text(lblBatteryStats, "Reading S3 battery...");
  lv_obj_set_style_text_color(lblBatteryStats, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_width(lblBatteryStats, lv_pct(100));

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
  snprintf(path, pathSize, "%s/tiles/%d/%ld/%ld.rgb565", SD_DIR, zoom, x, y);
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
      header.height != MAP_PLOT_H) {
    file.close();
    strlcpy(mapCacheStatus, "header mismatch", sizeof(mapCacheStatus));
    Serial.printf("[MAPCACHE] %s magic=%08lX ver=%u size=%ux%u expected=%ux%u\n",
                  mapCacheStatus,
                  (unsigned long)header.magic,
                  (unsigned)header.version,
                  (unsigned)header.width,
                  (unsigned)header.height,
                  (unsigned)MAP_PLOT_W,
                  (unsigned)MAP_PLOT_H);
    return;
  }
  size_t expected = sizeof(mapCanvasBuf);
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
  header.height = MAP_PLOT_H;
  header.zoom = cachedMapZoom;
  header.tileX = cachedMapTileX;
  header.tileY = cachedMapTileY;
  header.pixelX = cachedMapPixelX;
  header.pixelY = cachedMapPixelY;
  header.lat = cachedMapLat;
  header.lon = cachedMapLon;
  header.tileFound = cachedMapTileFound ? 1 : 0;
  size_t headerWritten = file.write((const uint8_t*)&header, sizeof(header));
  size_t imageWritten = file.write((const uint8_t*)mapCanvasBuf, sizeof(mapCanvasBuf));
  file.close();
  strlcpy(mapCacheStatus, imageWritten == sizeof(mapCanvasBuf) ? "saved" : "save short write", sizeof(mapCacheStatus));
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
  if (!mapCanvas || !sdStorage.available) return false;

  double centerX = lonToGlobalPixelX(lon, zoom);
  double centerY = latToGlobalPixelY(lat, zoom);
  long centerTileX = (long)floor(centerX / MAP_TILE_SIZE);
  long centerTileY = (long)floor(centerY / MAP_TILE_SIZE);
  bool centerTileFound = mapTileExists(zoom, centerTileX, centerTileY, centerPath, centerPathSize);

  int centerPixelX = (int)round(centerX);
  int centerPixelY = (int)round(centerY);
  if (mapCanvasCached &&
      cachedMapZoom == zoom &&
      cachedMapTileX == centerTileX &&
      cachedMapTileY == centerTileY &&
      abs(cachedMapPixelX - centerPixelX) < 32 &&
      abs(cachedMapPixelY - centerPixelY) < 32 &&
      cachedMapTileFound == centerTileFound) {
    return centerTileFound;
  }

  for (int y = 0; y < MAP_PLOT_H; y++) {
    for (int x = 0; x < MAP_PLOT_W; x++) {
      mapCanvasBuf[y * MAP_PLOT_W + x] = lv_color_hex(0x07100D);
    }
  }

  for (int y = 0; y < MAP_PLOT_H; y++) {
    long globalY = centerPixelY - (MAP_PLOT_H / 2) + y;
    if (globalY < 0) continue;
    long tileY = globalY / MAP_TILE_SIZE;
    int inTileY = globalY % MAP_TILE_SIZE;
    int x = 0;
    while (x < MAP_PLOT_W) {
      long globalX = centerPixelX - (MAP_PLOT_W / 2) + x;
      if (globalX < 0) {
        x++;
        continue;
      }
      long tileX = globalX / MAP_TILE_SIZE;
      int inTileX = globalX % MAP_TILE_SIZE;
      int segment = min(MAP_PLOT_W - x, MAP_TILE_SIZE - inTileX);
      char path[96];
      if (mapTileExists(zoom, tileX, tileY, path, sizeof(path))) {
        File tile = SD_MMC.open(path, FILE_READ);
        if (tile) {
          size_t bytesToRead = segment * sizeof(uint16_t);
          tile.seek((inTileY * MAP_TILE_SIZE + inTileX) * sizeof(uint16_t));
          size_t bytesRead = tile.read((uint8_t*)mapReadBuf, bytesToRead);
          tile.close();
          int pixelsRead = bytesRead / sizeof(uint16_t);
          for (int i = 0; i < pixelsRead; i++) {
            mapCanvasBuf[y * MAP_PLOT_W + x + i] = rgb565ToLvColor(mapReadBuf[i]);
          }
        }
      }
      x += segment;
    }
  }

  cachedMapZoom = zoom;
  cachedMapTileX = centerTileX;
  cachedMapTileY = centerTileY;
  cachedMapPixelX = centerPixelX;
  cachedMapPixelY = centerPixelY;
  cachedMapLat = lat;
  cachedMapLon = lon;
  cachedMapTileFound = centerTileFound;
  mapCanvasCached = true;
  saveMapCacheToSd();
  lv_obj_invalidate(mapCanvas);
  return centerTileFound;
}

static bool placeMapDot(size_t index, double lat, double lon, int zoom, lv_color_t color, int size) {
  if (index >= MAP_DOT_COUNT || !mapDots[index] || !gpsStats.valid) return false;
  double centerX = lonToGlobalPixelX(gpsStats.longitude, zoom);
  double centerY = latToGlobalPixelY(gpsStats.latitude, zoom);
  double pointX = lonToGlobalPixelX(lon, zoom);
  double pointY = latToGlobalPixelY(lat, zoom);
  int x = (MAP_PLOT_W / 2) + (int)round(pointX - centerX) - (size / 2);
  int y = (MAP_PLOT_H / 2) + (int)round(pointY - centerY) - (size / 2);
  if (x < -size || y < -size || x > MAP_PLOT_W || y > MAP_PLOT_H) {
    lv_obj_add_flag(mapDots[index], LV_OBJ_FLAG_HIDDEN);
    return false;
  }
  lv_obj_set_size(mapDots[index], size, size);
  lv_obj_set_pos(mapDots[index], x, y);
  lv_obj_set_style_bg_color(mapDots[index], color, 0);
  lv_obj_clear_flag(mapDots[index], LV_OBJ_FLAG_HIDDEN);
  return true;
}

static void refreshMapUi() {
  if (!mapPlot || !lblMapStats) return;
  if (currentPage != pageGps) return;
  uint32_t minIntervalMs = mapRenderPending ? 750 : 5000;
  if (lastMapUiRefreshMs && millis() - lastMapUiRefreshMs < minIntervalMs) return;
  lastMapUiRefreshMs = millis();
  mapRenderPending = false;

  const bool hasLocalFix = gpsStats.valid;
  for (size_t i = 0; i < MAP_DOT_COUNT; i++) {
    if (mapDots[i]) lv_obj_add_flag(mapDots[i], LV_OBJ_FLAG_HIDDEN);
  }

  if (!hasLocalFix) {
    if (mapCanvas && !mapCanvasCached) {
      lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
    }
    char waitingText[160];
    if (mapCanvasCached) {
      snprintf(waitingText, sizeof(waitingText),
               "Waiting for CYD GPS fix...\n"
               "Showing cached %.5f, %.5f z%d\n"
               "Cache: %s\n"
               "RX GPIO%d: %lu bytes",
               cachedMapLat,
               cachedMapLon,
               cachedMapZoom,
               mapCacheStatus,
               GPS_RX_PIN,
               (unsigned long)gpsBytesFromLocal);
    } else {
      snprintf(waitingText, sizeof(waitingText),
               "Waiting for CYD GPS fix...\n"
               "Cache: %s\n"
               "RX GPIO%d: %lu bytes\nSD maps: %s",
               mapCacheStatus,
               GPS_RX_PIN,
               (unsigned long)gpsBytesFromLocal,
               sdStorage.available ? "ready" : sdStorage.status);
    }
    lv_label_set_text(lblMapStats, waitingText);
    return;
  }

  char tilePath[96];
  int mapZoom = findBestMapZoom(gpsStats.latitude, gpsStats.longitude, tilePath, sizeof(tilePath));
  bool centerTileFound = renderOfflineTileMap(gpsStats.latitude, gpsStats.longitude, mapZoom, tilePath, sizeof(tilePath));
  size_t plotted = 0;
  for (size_t i = 0; i < nodeCount; i++) {
    if (!nodes[i].hasPosition) continue;
    lv_color_t color = nodes[i].num == stats.myNodeNum ? lv_color_hex(0x00C985) : lv_color_hex(0x68FFC0);
    if (placeMapDot(i, nodes[i].latitude, nodes[i].longitude, mapZoom, color, 8)) plotted++;
  }

  placeMapDot(MAP_DOT_COUNT - 1, gpsStats.latitude, gpsStats.longitude, mapZoom, lv_color_hex(0x00C985), 10);
  plotted++;

  long tileX = lonToTileX(gpsStats.longitude, mapZoom);
  long tileY = latToTileY(gpsStats.latitude, mapZoom);

  char mapText[280];
  snprintf(mapText, sizeof(mapText),
           "CYD GPS: fix  RX %lu bytes\n"
           "Lat/lon: %.5f, %.5f\n"
           "Offline tile z%d/%ld/%ld: %s\n"
           "Cache: %s\n"
           "%u plotted point%s",
           (unsigned long)gpsBytesFromLocal,
           gpsStats.latitude,
           gpsStats.longitude,
           mapZoom,
           tileX,
           tileY,
           centerTileFound ? "drawn" : "missing",
           mapCacheStatus,
           (unsigned)plotted,
           plotted == 1 ? "" : "s");
  lv_label_set_text(lblMapStats, mapText);
}

static void refreshScreenUi() {
  if (!lblStatus || millis() - lastUiRefreshMs < 500) return;
  lastUiRefreshMs = millis();
  refreshSdUsage();

  char status[48];
  if (localGps.time.isValid()) {
    int cdtHour = (localGps.time.hour() + 19) % 24;
    snprintf(status, sizeof(status), "%02d:%02d CDT", cdtHour, localGps.time.minute());
  } else {
    snprintf(status, sizeof(status), "Wait GPS");
  }
  lv_label_set_text(lblStatus, status);

  if (lblBatteryStatus) {
    char batteryStatus[24];
    snprintf(batteryStatus, sizeof(batteryStatus), "Batt %d%%", localBattery.percent);
    lv_label_set_text(lblBatteryStatus, batteryStatus);
  }

  char statsText[256];
  snprintf(statsText, sizeof(statsText),
           "Node: %s\nID: !%08lX\nRX/TX: %lu/%lu  Nodes: %u/%u\nPriv: %s",
           nodeName(stats.myNodeNum),
           (unsigned long)stats.myNodeNum,
           (unsigned long)stats.packetsRx,
           (unsigned long)stats.packetsTx,
           stats.onlineNodes,
           stats.totalNodes,
           privateChannelIndex >= 0 ? "found" : "not found");
  lv_label_set_text(lblStats, statsText);

  if (lblSystemInterface) {
    String apIp = wifiEnabled ? WiFi.softAPIP().toString() : String("off");
    char interfaceText[360];
    snprintf(interfaceText, sizeof(interfaceText),
             "S3 interface\n"
             "Uptime: %lu s\n"
             "AP: %s\n\n"
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
             apIp.c_str(),
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

  if (lblSystemSerial) {
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

  if (lblSystemRadio) {
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

  if (swWifiEnabled) {
    if (wifiEnabled) lv_obj_add_state(swWifiEnabled, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiEnabled, LV_STATE_CHECKED);
  }

  if (lblWifiState) {
    char wifiStateText[160];
    snprintf(wifiStateText, sizeof(wifiStateText),
             "Status: %s\nSSID: %s\nIP: %s",
             wifiEnabled ? "on" : "off",
             wifiEnabled ? INTERFACE_AP_SSID : "-",
             wifiEnabled ? WiFi.softAPIP().toString().c_str() : "-");
    lv_label_set_text(lblWifiState, wifiStateText);
  }

  if (lblWifiStats) {
    uint32_t wifiAge = 0;
    if (wifiEnabled && wifiStartedMs) wifiAge = (millis() - wifiStartedMs) / 1000;
    else if (!wifiEnabled && wifiStoppedMs) wifiAge = (millis() - wifiStoppedMs) / 1000;

    char wifiStatsText[520];
    snprintf(wifiStatsText, sizeof(wifiStatsText),
             "Access point\n"
             "State: %s\n"
             "SSID: %s\n"
             "IP: %s\n"
             "MAC: %s\n"
             "Stations: %u\n"
             "Channel: %d\n\n"
             "Runtime\n"
             "%s for: %lu s\n"
             "Toggles: %lu\n\n"
             "Web server\n"
             "State: %s\n"
             "Port: 80",
             wifiEnabled ? "on" : "off",
             wifiEnabled ? INTERFACE_AP_SSID : "-",
             wifiEnabled ? WiFi.softAPIP().toString().c_str() : "-",
             wifiEnabled ? WiFi.softAPmacAddress().c_str() : "-",
             wifiEnabled ? WiFi.softAPgetStationNum() : 0,
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

  if (lblBacklight) {
    char backlightText[96];
    snprintf(backlightText, sizeof(backlightText),
             "Brightness: %u%%\nPWM: %s",
             backlightPercent,
             backlightPwmReady ? "LEDC" : "analog fallback");
    lv_label_set_text(lblBacklight, backlightText);
  }

  if (lblBatteryStats) {
    char batteryText[420];
    snprintf(batteryText, sizeof(batteryText),
             "S3 battery\n"
             "Level: %d%%\n"
             "Battery estimate: %.2f V\n"
             "Sense voltage: %.2f V\n"
             "ADC pin: GPIO%d\n"
             "ADC reading: %lu mV\n"
             "Power: %s\n"
             "Trend: %ld mV/min\n\n"
             "S3 interface power\n"
             "Uptime: %lu s\n"
             "Heap free/min: %lu/%lu KB\n"
             "PSRAM free: %lu KB\n\n"
             "Heltec radio\n"
             "Packets RX/TX: %lu/%lu\n"
             "Channel use: %.2f%%\n"
             "Air TX use: %.2f%%",
             localBattery.percent,
             localBattery.batteryMv / 1000.0f,
             localBattery.rawPackMv / 1000.0f,
             BATT_ADC_PIN,
             (unsigned long)localBattery.rawMv,
             localBattery.powerState,
             (long)localBattery.deltaMvPerMin,
             (unsigned long)(millis() / 1000),
             (unsigned long)(ESP.getFreeHeap() / 1024),
             (unsigned long)(ESP.getMinFreeHeap() / 1024),
             (unsigned long)(ESP.getFreePsram() / 1024),
             (unsigned long)stats.packetsRx,
             (unsigned long)stats.packetsTx,
             stats.channelUtilization,
             stats.airUtilTx);
    lv_label_set_text(lblBatteryStats, batteryText);
  }

  if (lblGpsStats) {
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

  if (taPublicChat) lv_textarea_set_text(taPublicChat, publicChatLog[0] ? publicChatLog : "No public chat yet");
  if (taPrivateChat) lv_textarea_set_text(taPrivateChat, privateChatLog[0] ? privateChatLog : "No private chat yet");
  if (taScreenLog) lv_textarea_set_text(taScreenLog, eventLog[0] ? eventLog : "Waiting for radio data");

  if (taScreenNodes) {
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
    lv_textarea_set_text(taScreenNodes, text.length() ? text.c_str() : "No nodes heard yet");
  }
}

static void serviceScreen() {
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

static NodeRecord* findOrCreateNode(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num) return &nodes[i];
  }
  if (nodeCount >= MAX_NODES) return nullptr;
  nodes[nodeCount].num = num;
  snprintf(nodes[nodeCount].name, sizeof(nodes[nodeCount].name), "!%08lX", (unsigned long)num);
  nodes[nodeCount].lastHeardMs = millis();
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
  return index == PUBLIC_CHANNEL_INDEX ? "primary" : "unknown";
}

static void updateChannelRecord(const meshtastic_Channel& channel) {
  if (channel.index < 0 || channel.index >= (int8_t)MAX_CHANNELS) return;
  ChannelRecord& record = channels[channel.index];
  record.index = channel.index;
  record.enabled = channel.role != meshtastic_Channel_Role_DISABLED;
  if (channel.has_settings && channel.settings.name[0]) {
    strlcpy(record.name, channel.settings.name, sizeof(record.name));
  } else if (channel.index == PUBLIC_CHANNEL_INDEX) {
    strlcpy(record.name, "primary", sizeof(record.name));
  } else {
    record.name[0] = '\0';
  }
  if (record.enabled && strcmp(record.name, "priv") == 0) {
    privateChannelIndex = record.index;
  }

  char line[96];
  snprintf(line, sizeof(line), "[radio] channel %d: %s\n", record.index, record.name[0] ? record.name : "(unnamed)");
  appendLine(eventLog, LOG_SIZE, line);
}

static bool isPrivateChannel(uint8_t index) {
  if (privateChannelIndex >= 0) return index == privateChannelIndex;
  return index == 1;
}

static void refreshChatViews() {
  if (taPublicChat) lv_textarea_set_text(taPublicChat, publicChatLog[0] ? publicChatLog : "No public chat yet");
  if (taPrivateChat) lv_textarea_set_text(taPrivateChat, privateChatLog[0] ? privateChatLog : "No private chat yet");
}

static void rememberLocalSentText(uint8_t channel, const char* text, size_t len) {
  size_t copyLen = min(len, sizeof(lastLocalSentText) - 1);
  memcpy(lastLocalSentText, text, copyLen);
  lastLocalSentText[copyLen] = '\0';
  lastLocalSentChannel = channel;
  lastLocalSentMs = millis();
}

static bool isRecentLocalEcho(uint8_t channel, const uint8_t* text, size_t len) {
  if (!lastLocalSentMs || channel != lastLocalSentChannel) return false;
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

static void appendChatMessage(uint8_t channel, const char* sender, const uint8_t* text, size_t len) {
  if (!text || len == 0) return;
  char stamp[28];
  formatChatTimestamp(stamp, sizeof(stamp));
  char line[320];
  snprintf(line, sizeof(line), "[%s] [%s] %.*s\n", stamp, sender && sender[0] ? sender : "unknown", (int)len, text);
  appendLine(isPrivateChannel(channel) ? privateChatLog : publicChatLog, CHAT_SIZE, line);
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
  packet.to = 0xFFFFFFFF;
  packet.channel = channelIndex < 0 ? PUBLIC_CHANNEL_INDEX : channelIndex;
  packet.want_ack = false;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  writeStreamFrame(out, stream.bytes_written);

  rememberLocalSentText(packet.channel, text, data.payload.size);
  appendChatMessage(packet.channel, "me", data.payload.bytes, data.payload.size);

  char line[96];
  snprintf(line, sizeof(line), "[local] text sent on %s\n", channelName(packet.channel));
  appendLine(eventLog, LOG_SIZE, line);
  return true;
}

static void updateTelemetry(uint32_t from, const meshtastic_Data& data) {
  meshtastic_Telemetry telemetry = meshtastic_Telemetry_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
  if (!pb_decode(&stream, meshtastic_Telemetry_fields, &telemetry)) return;

  char line[160];
  if (telemetry.which_variant == meshtastic_Telemetry_device_metrics_tag) {
    lastTelemetryMs = millis();
    stats.batteryLevel = telemetry.variant.device_metrics.battery_level;
    stats.voltage = telemetry.variant.device_metrics.voltage;
    stats.channelUtilization = telemetry.variant.device_metrics.channel_utilization;
    stats.airUtilTx = telemetry.variant.device_metrics.air_util_tx;
    stats.uptimeSeconds = telemetry.variant.device_metrics.uptime_seconds;
    snprintf(line, sizeof(line), "[%s] battery %lu%% %.2fV\n",
             nodeName(from), (unsigned long)stats.batteryLevel, stats.voltage);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (telemetry.which_variant == meshtastic_Telemetry_local_stats_tag) {
    lastTelemetryMs = millis();
    stats.packetsRx = telemetry.variant.local_stats.num_packets_rx;
    stats.packetsTx = telemetry.variant.local_stats.num_packets_tx;
    stats.onlineNodes = telemetry.variant.local_stats.num_online_nodes;
    stats.totalNodes = telemetry.variant.local_stats.num_total_nodes;
    appendLine(eventLog, LOG_SIZE, "[radio] local stats updated\n");
  }
}

static void handleDecodedPacket(const meshtastic_MeshPacket& packet) {
  NodeRecord* node = findOrCreateNode(packet.from);
  if (node) {
    node->lastHeardMs = millis();
    node->snr = packet.rx_snr;
  }

  if (packet.which_payload_variant == meshtastic_MeshPacket_encrypted_tag) {
    encryptedPackets++;
    char line[96];
    snprintf(line, sizeof(line), "[%s] encrypted packet\n", nodeName(packet.from));
    appendLine(eventLog, LOG_SIZE, line);
    return;
  }

  if (packet.which_payload_variant != meshtastic_MeshPacket_decoded_tag) return;

  const meshtastic_Data& data = packet.decoded;
  char line[320];

  if (data.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP) {
    textPackets++;
    lastPortNum = data.portnum;
    bool fromThisNode = stats.myNodeNum != 0 && packet.from == stats.myNodeNum;
    if (!(fromThisNode && isRecentLocalEcho(packet.channel, data.payload.bytes, data.payload.size))) {
      appendChatMessage(packet.channel, fromThisNode ? "me" : nodeName(packet.from), data.payload.bytes, data.payload.size);
    }
    snprintf(line, sizeof(line), "[%s] %.*s\n", fromThisNode ? "me" : nodeName(packet.from),
             (int)data.payload.size, data.payload.bytes);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (data.portnum == meshtastic_PortNum_TELEMETRY_APP) {
    telemetryPackets++;
    lastPortNum = data.portnum;
    updateTelemetry(packet.from, data);
  } else if (data.portnum == meshtastic_PortNum_POSITION_APP) {
    positionPackets++;
    lastPortNum = data.portnum;
    appendLine(eventLog, LOG_SIZE, "[radio] Heltec position packet ignored; using CYD GPS UART\n");
  } else if (data.portnum == meshtastic_PortNum_NODEINFO_APP) {
    nodeInfoPackets++;
    lastPortNum = data.portnum;
    meshtastic_User user = meshtastic_User_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
    if (pb_decode(&stream, meshtastic_User_fields, &user)) {
      NodeRecord* known = findOrCreateNode(packet.from);
      if (known) {
        strncpy(known->name, user.long_name, sizeof(known->name) - 1);
        known->name[sizeof(known->name) - 1] = '\0';
      }
      snprintf(line, sizeof(line), "[radio] node %08lX is %.39s\n",
               (unsigned long)packet.from, user.long_name);
      appendLine(eventLog, LOG_SIZE, line);
    }
  } else {
    lastPortNum = data.portnum;
    otherFrames++;
    snprintf(line, sizeof(line), "[%s] port %d payload %u bytes\n",
             nodeName(packet.from), data.portnum, data.payload.size);
    appendLine(eventLog, LOG_SIZE, line);
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
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_node_info_tag) {
    nodeInfoPackets++;
    NodeRecord* node = findOrCreateNode(fromRadio.node_info.num);
    if (node) {
      strncpy(node->name, fromRadio.node_info.user.long_name, sizeof(node->name) - 1);
      node->name[sizeof(node->name) - 1] = '\0';
      node->snr = fromRadio.node_info.snr;
      node->lastHeardMs = millis();
    }
    if (fromRadio.node_info.has_position) {
      positionPackets++;
      appendLine(eventLog, LOG_SIZE, "[radio] Heltec node-info position ignored; using CYD GPS UART\n");
    }
    if (fromRadio.node_info.has_device_metrics) {
      lastTelemetryMs = millis();
      stats.batteryLevel = fromRadio.node_info.device_metrics.battery_level;
      stats.voltage = fromRadio.node_info.device_metrics.voltage;
      stats.channelUtilization = fromRadio.node_info.device_metrics.channel_utilization;
      stats.airUtilTx = fromRadio.node_info.device_metrics.air_util_tx;
      stats.uptimeSeconds = fromRadio.node_info.device_metrics.uptime_seconds;
    }
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_log_record_tag) {
    char line[420];
    snprintf(line, sizeof(line), "[node log] %.380s\n", fromRadio.log_record.message);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_channel_tag) {
    configFrames++;
    updateChannelRecord(fromRadio.channel);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_config_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_moduleConfig_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_config_complete_id_tag) {
    configFrames++;
  } else {
    otherFrames++;
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

static void handleStatus() {
  sampleLocalBattery();
  refreshSdUsage();
  char rxAge[32];
  if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lus ago", (unsigned long)((millis() - lastByteMs) / 1000));
  else strlcpy(rxAge, "never", sizeof(rxAge));
  String json = "{";
  json += "\"title\":\"" + String(UI::Labels::AppTitle) + "\",";
  json += "\"ip\":\"" + String(wifiEnabled ? WiFi.softAPIP().toString() : String("off")) + "\",";
  json += "\"wifiEnabled\":" + String(wifiEnabled ? "true" : "false") + ",";
  json += "\"wifiStations\":" + String(wifiEnabled ? WiFi.softAPgetStationNum() : 0) + ",";
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
  json += "\"senseVoltage\":" + String(localBattery.rawPackMv / 1000.0f, 2) + ",";
  json += "\"batterySource\":\"S3\",";
  json += "\"powerState\":\"" + jsonEscape(localBattery.powerState) + "\",";
  json += "\"batteryTrend\":" + String(localBattery.deltaMvPerMin) + ",";
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
  json += "\"privateChat\":\"" + jsonEscape(privateChatLog) + "\",";
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
  json += "\"log\":\"" + jsonEscape(eventLog) + "\",";
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
  server.send(200, "application/json", json);
}

static void handleSdDownload(const char* path, const char* downloadName, const char* contentType) {
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
  if (!server.hasArg("msg")) {
    server.send(400, "text/plain", "missing msg");
    return;
  }
  bool ok = sendTextMessage(server.arg("msg").c_str(), PUBLIC_CHANNEL_INDEX);
  server.send(ok ? 200 : 500, "text/plain", ok ? "sent" : "send failed");
}

static void handleSerialCmd() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "missing cmd");
    return;
  }
  String cmd = server.arg("cmd") + "\n";
  SerialLoRa.write((const uint8_t*)cmd.c_str(), cmd.length());
  server.send(200, "text/plain", "sent");
}

static void handleRoot() {
  server.send(200, "text/html", R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Heltec LoRa Interface</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
<style>
*{box-sizing:border-box}html,body{min-height:100%;background:#050807;color:#f4fff9}
body{margin:0;background:#050807!important;color:#f4fff9!important;font-family:system-ui,Segoe UI,sans-serif}
header{padding:8px 10px;background:#080d0b;border-bottom:1px solid #24483e}
h1{font-size:16px;margin:0;color:#f4fff9}main{display:grid;gap:8px;padding:8px;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
section{background:#101816;color:#f4fff9;border:1px solid #24483e;border-radius:6px;padding:8px}
h2{font-size:12px;line-height:1.1;margin:0 0 6px;color:#68ffc0;text-transform:uppercase}.stats{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.stat{background:#07100d;color:#f4fff9;padding:6px;border-radius:4px}.label{color:#8ab7a6;font-size:11px}.value{font-size:15px;color:#f4fff9}
pre{white-space:pre-wrap;overflow:auto;max-height:280px;margin:0;color:#e8fff5;background:#07100d;font:12px ui-monospace,Consolas,monospace}
input{box-sizing:border-box;width:100%;padding:10px;background:#07100d!important;border:1px solid #2f705f;color:#fff!important;border-radius:6px}
button{margin-top:6px;padding:9px 12px;border:0;border-radius:6px;background:#00c985;color:#001b12;font-weight:700}
a{color:#68ffc0;text-decoration:none}.links{display:flex;flex-wrap:wrap;gap:6px}.links a{padding:7px 8px;border:1px solid #2f705f;border-radius:6px;background:#07100d}
table{width:100%;border-collapse:collapse;font-size:12px;color:#f4fff9}td,th{border-bottom:1px solid #203b35;padding:4px;text-align:left}
#realMap{width:100%;height:320px;background:#07100d;border:1px solid #24483e;border-radius:6px;box-sizing:border-box;overflow:hidden}
canvas{width:100%;height:260px;background:#07100d;border:1px solid #24483e;border-radius:6px;box-sizing:border-box}
.hidden{display:none}
.mapMeta{color:#8ab7a6;font-size:12px;margin-top:8px}
.leaflet-container{background:#07100d;color:#10231d}.leaflet-popup-content-wrapper,.leaflet-popup-tip{background:#101816;color:#e8fff5}
</style></head><body><header><h1>Heltec LoRa Interface</h1><div id="ip"></div></header><main>
<section><h2>Radio</h2><div class="stats" id="stats"></div><button onclick="fetch('/config',{method:'POST'})">Request Config</button></section>
<section><h2>Send Message</h2><input id="msg" maxlength="233" placeholder="Message to mesh"><button onclick="send()">Send</button></section>
<section><h2>Chat</h2><pre id="chat"></pre></section>
<section><h2>Map</h2><div id="realMap"></div><canvas id="mapFallback" class="hidden" width="640" height="360"></canvas><div class="mapMeta" id="mapMeta"></div></section>
<section><h2>SD Storage</h2><div class="stats" id="storage"></div><button onclick="mountSd()">Mount SD</button><div class="links"><a href="/sd/events">Events</a><a href="/sd/public">Public Chat</a><a href="/sd/private">Private Chat</a><a href="/sd/positions">Positions CSV</a><a href="/sd/mapcache">Map Cache</a><a href="/sd/last-location">Last GPS</a></div></section>
<section><h2>Nodes</h2><table><thead><tr><th>Node</th><th>Name</th><th>SNR</th><th>Age</th><th>GPS</th></tr></thead><tbody id="nodes"></tbody></table></section>
<section><h2>Serial Link</h2><input id="cmd" placeholder="Serial command"><button onclick="sendCmd()">Send Command</button><pre id="serial" style="margin-top:6px"></pre></section>
<section><h2>Event Log</h2><pre id="log"></pre></section>
</main><script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script><script>
let leafletMap=null,markers={},realMapReady=false;
function initRealMap(){if(realMapReady||!window.L)return;leafletMap=L.map('realMap',{zoomControl:true,attributionControl:true});L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:19,attribution:'&copy; OpenStreetMap'}).addTo(leafletMap);leafletMap.setView([0,0],2);realMapReady=true;}
async function refresh(){const s=await (await fetch('/status')).json();ip.textContent='AP '+s.ip;
stats.innerHTML=[['Node',s.myNode],['S3 Battery',s.battery+'% '+s.voltage+'V'],['Power',s.powerState],['Frames',s.frames],['Errors',s.errors],['RX/TX',s.rx+'/'+s.tx],['Nodes',s.online+'/'+s.total],['SD',s.sdStatus]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
storage.innerHTML=[['Status',s.sdAvailable?'mounted':s.sdStatus],['Type',s.sdType],['Used',formatBytes(s.sdUsedKb)],['Total',formatBytes(s.sdTotalKb)],['Writes',s.sdWrites],['Errors',s.sdErrors],['Map Cache',s.mapCacheLoaded?'loaded':s.mapCacheStatus]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
chat.textContent=s.chat||'No chat yet';log.textContent=s.log||'Waiting for radio data';
serial.textContent=`RX bytes: ${s.bytes}\nTX bytes: ${s.txBytes}\nLast byte: ${s.lastByte}\nMagic 94/C3: ${s.magic1}/${s.magic2}\nStream frames: ${s.streamFrames}\nBad lengths: ${s.badLengths}\nText/Tel/GPS/Node: ${s.textPackets}/${s.telemetryPackets}/${s.positionPackets}/${s.nodeInfoPackets}\nHeltec GPS ignored: ${s.remotePositionPackets}\nConfig/Other/Encrypted: ${s.configFrames}/${s.otherFrames}/${s.encryptedPackets}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}`;
nodes.innerHTML=s.nodes.map(n=>`<tr><td>${n.num}</td><td>${n.name}</td><td>${n.snr}</td><td>${n.age}s</td><td>${n.hasPosition?`${n.lat.toFixed(5)}, ${n.lon.toFixed(5)}`:'-'}</td></tr>`).join('');
drawMap(s);
}
async function send(){const m=msg.value.trim();if(!m)return;await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(m)});msg.value='';refresh();}
async function sendCmd(){const c=cmd.value.trim();if(!c)return;await fetch('/serial_cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(c)});cmd.value='';refresh();}
async function mountSd(){await fetch('/sd/mount',{method:'POST'});refresh();}
function formatBytes(kb){if(!kb)return'0 KB';return kb>=1024?Math.ceil(kb/1024)+' MB':kb+' KB';}
function drawMap(s){initRealMap();const pts=s.nodes.filter(n=>n.hasPosition);if(realMapReady){realMap.classList.remove('hidden');mapFallback.classList.add('hidden');drawRealMap(s,pts);return}realMap.classList.add('hidden');mapFallback.classList.remove('hidden');drawFallbackMap(s,pts);}
function drawRealMap(s,pts){if(!pts.length){mapMeta.textContent='No node positions yet';return}const seen={};for(const p of pts){seen[p.num]=true;const html=`<b>${p.name||p.num}</b><br>${p.num}<br>${p.lat.toFixed(6)}, ${p.lon.toFixed(6)}<br>Alt ${p.alt} m<br>${p.positionAge}s old`;if(!markers[p.num])markers[p.num]=L.circleMarker([p.lat,p.lon],{radius:7,color:p.num===s.myNode?'#00c985':'#68ffc0',weight:2,fillColor:p.num===s.myNode?'#00c985':'#68ffc0',fillOpacity:.85}).addTo(leafletMap);else markers[p.num].setLatLng([p.lat,p.lon]);markers[p.num].setStyle({color:p.num===s.myNode?'#00c985':'#68ffc0',fillColor:p.num===s.myNode?'#00c985':'#68ffc0'});markers[p.num].bindPopup(html)}for(const id in markers){if(!seen[id]){leafletMap.removeLayer(markers[id]);delete markers[id]}}const bounds=L.latLngBounds(pts.map(p=>[p.lat,p.lon]));leafletMap.fitBounds(bounds.pad(.2),{maxZoom:15,animate:false});mapMeta.textContent=`OpenStreetMap | ${pts.length} positioned node${pts.length===1?'':'s'}`;}
function drawFallbackMap(s,pts){const c=mapFallback,ctx=c.getContext('2d');ctx.clearRect(0,0,c.width,c.height);ctx.fillStyle='#07100d';ctx.fillRect(0,0,c.width,c.height);ctx.strokeStyle='#1f3d35';ctx.lineWidth=1;for(let x=40;x<c.width;x+=80){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,c.height);ctx.stroke()}for(let y=40;y<c.height;y+=80){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(c.width,y);ctx.stroke()}if(!pts.length){ctx.fillStyle='#8ab7a6';ctx.font='16px system-ui';ctx.fillText('Waiting for position packets',24,40);mapMeta.textContent='No node positions yet';return}let minLat=Math.min(...pts.map(p=>p.lat)),maxLat=Math.max(...pts.map(p=>p.lat)),minLon=Math.min(...pts.map(p=>p.lon)),maxLon=Math.max(...pts.map(p=>p.lon));let latSpan=Math.max(maxLat-minLat,0.000001),lonSpan=Math.max(maxLon-minLon,0.000001),pad=28;for(const p of pts){let x=pad+(p.lon-minLon)*(c.width-pad*2)/lonSpan,y=pad+(maxLat-p.lat)*(c.height-pad*2)/latSpan;ctx.fillStyle=p.num===s.myNode?'#00c985':'#68ffc0';ctx.beginPath();ctx.arc(x,y,6,0,Math.PI*2);ctx.fill();ctx.fillStyle='#e8fff5';ctx.font='12px system-ui';ctx.fillText(p.name||p.num,x+9,y+4)}mapMeta.textContent=`Offline plot | ${pts.length} positioned node${pts.length===1?'':'s'} | center ${((minLat+maxLat)/2).toFixed(5)}, ${((minLon+maxLon)/2).toFixed(5)}`;}
setInterval(refresh,1000);refresh();
</script></body></html>
)HTML");
}

void setup() {
  Serial.begin(115200);
  SerialLoRa.setRxBufferSize(4096);
  SerialLoRa.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  initScreen();
  initSdStorage();

  appendLine(eventLog, LOG_SIZE, "[boot] Heltec LoRa interface starting\n");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/serial_cmd", HTTP_POST, handleSerialCmd);
  server.on("/sd/events", HTTP_GET, []() { handleSdDownload(SD_EVENTS_PATH, "events.log", "text/plain"); });
  server.on("/sd/public", HTTP_GET, []() { handleSdDownload(SD_PUBLIC_CHAT_PATH, "public_chat.log", "text/plain"); });
  server.on("/sd/private", HTTP_GET, []() { handleSdDownload(SD_PRIVATE_CHAT_PATH, "private_chat.log", "text/plain"); });
  server.on("/sd/positions", HTTP_GET, []() { handleSdDownload(SD_POSITIONS_PATH, "positions.csv", "text/csv"); });
  server.on("/sd/mapcache", HTTP_GET, []() { handleSdDownload(SD_MAP_CACHE_PATH, "map_cache.bin", "application/octet-stream"); });
  server.on("/sd/last-location", HTTP_GET, []() { handleSdDownload(SD_LAST_LOCATION_PATH, "last_location.txt", "text/plain"); });
  server.on("/sd/mount", HTTP_POST, []() {
    initSdStorage();
    server.send(sdStorage.available ? 200 : 503, "text/plain", sdStorage.status);
  });
  server.on("/config", HTTP_POST, []() {
    bool ok = sendConfigRequest();
    server.send(ok ? 200 : 500, "text/plain", ok ? "requested" : "request failed");
  });
  startWifiAp();

  char line[128];
  snprintf(line, sizeof(line), "[boot] AP %s at %s\n",
           wifiEnabled ? INTERFACE_AP_SSID : "off",
           wifiEnabled ? WiFi.softAPIP().toString().c_str() : "0.0.0.0");
  appendLine(eventLog, LOG_SIZE, line);
  sendConfigRequest();
}

void loop() {
  pollLocalGps();
  pollLoRa();
  serviceConfigRequests();
  if (wifiEnabled) server.handleClient();
  serviceScreen();
  printSerialDiagnostics();
  delay(2);
}

// Internal implementation module; included once by the main sketch.
static void initSdMutex() {
  if (!sdMutex) {
    sdMutex = xSemaphoreCreateRecursiveMutex();
    Serial.printf("[sd] mutex %s\n", sdMutex ? "ready" : "failed");
  }
}

static bool lockSd(uint32_t timeoutMs) {
  if (!sdMutex) return true;
  if (xSemaphoreTakeRecursive(sdMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE) return true;
  if (timeoutMs != 0) {
    sdLockTimeouts++;
    strlcpy(sdStorage.status, "busy", sizeof(sdStorage.status));
  }
  return false;
}

static void unlockSd() {
  if (sdMutex) xSemaphoreGiveRecursive(sdMutex);
}

struct SdLock {
  bool locked;
  explicit SdLock(uint32_t timeoutMs = 2000) : locked(lockSd(timeoutMs)) {}
  ~SdLock() {
    if (locked) unlockSd();
  }
  explicit operator bool() const {
    return locked;
  }
};

static bool writeSdLineNow(const char* path, const char* line);

static UBaseType_t sdAppendQueueWaiting() {
  return sdAppendQueue ? uxQueueMessagesWaiting(sdAppendQueue) : 0;
}

static void sdAppendTask(void*) {
  SdAppendItem item;
  for (;;) {
    if (xQueueReceive(sdAppendQueue, &item, portMAX_DELAY) == pdTRUE) {
      if (writeSdLineNow(item.path, item.line)) {
        sdAppendWritten++;
      } else {
        sdAppendDropped++;
      }
    }
  }
}

static void initSdAppendWorker() {
  if (!sdAppendQueue) {
    sdAppendQueue = xQueueCreate(SD_APPEND_QUEUE_DEPTH, sizeof(SdAppendItem));
    Serial.printf("[sd] append queue %s depth=%u item=%u\n",
                  sdAppendQueue ? "ready" : "failed",
                  (unsigned)SD_APPEND_QUEUE_DEPTH,
                  (unsigned)sizeof(SdAppendItem));
  }
  if (sdAppendQueue && !sdAppendTaskHandle) {
    BaseType_t ok = xTaskCreatePinnedToCore(sdAppendTask, "sdAppend", 6144, nullptr, 1, &sdAppendTaskHandle, 0);
    Serial.printf("[sd] append worker %s\n", ok == pdPASS ? "started" : "failed");
    if (ok != pdPASS) {
      sdAppendTaskHandle = nullptr;
      vQueueDelete(sdAppendQueue);
      sdAppendQueue = nullptr;
    }
  }
}

static void initMapRenderWorker() {
  if (!mapRenderResultQueue) {
    mapRenderResultQueue = xQueueCreate(1, sizeof(MapRenderResult));
    Serial.printf("[map] result queue %s\n", mapRenderResultQueue ? "ready" : "failed");
  }
}

static size_t loadSdTextTail(const char* path, char* buffer, size_t bufferSize) {
  SdLock lock;
  if (!lock) return 0;
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

static bool writeSdLineNow(const char* path, const char* line) {
  if (!sdStorage.available || !path || !line) return false;
  SdLock lock;
  if (!lock) {
    sdStorage.writeErrors++;
    return false;
  }
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

static bool appendSdLine(const char* path, const char* line) {
  if (!sdStorage.available || !path || !line) return false;

  if (sdAppendQueue && sdAppendTaskHandle) {
    SdAppendItem item = {};
    strlcpy(item.path, path, sizeof(item.path));
    size_t lineLen = strlcpy(item.line, line, sizeof(item.line));
    if (lineLen >= sizeof(item.line)) {
      sdAppendTruncated++;
      item.line[sizeof(item.line) - 2] = '\n';
      item.line[sizeof(item.line) - 1] = '\0';
    }
    if (xQueueSend(sdAppendQueue, &item, 0) == pdTRUE) {
      sdAppendQueued++;
      strlcpy(sdStorage.status, "queued", sizeof(sdStorage.status));
      return true;
    }
  }

  sdAppendFallbacks++;
  bool ok = writeSdLineNow(path, line);
  if (!ok) sdAppendDropped++;
  return ok;
}

static void initSdStorage() {
  SdLock lock(5000);
  if (!lock) {
    sdStorage.available = false;
    strlcpy(sdStorage.status, "mount busy", sizeof(sdStorage.status));
    Serial.println("[sd] mount skipped: busy");
    return;
  }
  mapStorageGeneration.fetch_add(1);
  mapTileCacheResident.store(0);
  mapCanvasCached = false;
  mapRenderPending = true;
  lastMapUiRefreshMs = 0;
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
  SdLock lock;
  if (!lock) return 0;
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
  SdLock lock;
  mapStorageGeneration.fetch_add(1);
  mapTileCacheResident.store(0);
  mapCanvasCached = false;
  mapRenderPending = true;
  lastMapUiRefreshMs = 0;
  if (!lock) {
    mapTileRootFound = false;
    strlcpy(mapTileRoot, DEFAULT_MAP_TILE_ROOT, sizeof(mapTileRoot));
    if (logResult) Serial.println("[tiles] root scan skipped: SD busy");
    return;
  }
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
  SdLock lock(10000);
  if (!lock) {
    Serial.println("[tileroots] SD busy");
    return;
  }
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
  SdLock lock(30000);
  if (!lock) {
    Serial.println("[tiles] SD busy");
    return;
  }
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
  SdLock lock(10000);
  if (!lock) {
    Serial.println("[tilesfast] SD busy");
    return;
  }
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
  SdLock lock;
  if (!lock) return;
  if (SD_MMC.exists(SD_LAST_LOCATION_PATH)) SD_MMC.remove(SD_LAST_LOCATION_PATH);
  File file = SD_MMC.open(SD_LAST_LOCATION_PATH, FILE_WRITE);
  if (!file) return;
  file.printf("%.7f,%.7f,%ld,%lu\n", lat, lon, (long)alt, (unsigned long)millis());
  file.close();
}

static bool loadLastLocationFromSd() {
  SdLock lock;
  if (!lock) return false;
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

// Internal implementation module; included once by the main sketch.
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
  SdLock lock;
  if (!lock) return false;
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
  SdLock lock;
  if (!lock) {
    strlcpy(mapCacheStatus, "SD busy", sizeof(mapCacheStatus));
    return;
  }
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
  if (sdDownloadPathActive(SD_MAP_CACHE_PATH)) return;
  SdLock lock;
  if (!lock) {
    strlcpy(mapCacheStatus, "save busy", sizeof(mapCacheStatus));
    return;
  }
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

static bool mapRenderJobMatches(const MapRenderJob& job,
                                double lat,
                                double lon,
                                int zoom,
                                uint16_t plotH,
                                long centerTileX,
                                long centerTileY,
                                int centerPixelX,
                                int centerPixelY) {
  return job.zoom == zoom &&
         job.plotH == plotH &&
         job.centerTileX == centerTileX &&
         job.centerTileY == centerTileY &&
         abs(job.centerPixelX - centerPixelX) < 2 &&
         abs(job.centerPixelY - centerPixelY) < 2 &&
         fabs(job.lat - lat) < 0.000001 &&
         fabs(job.lon - lon) < 0.000001;
}

static bool mapCacheMatchesCenter(double lat,
                                  double lon,
                                  int zoom,
                                  uint16_t plotH,
                                  long* centerTileX,
                                  long* centerTileY,
                                  int* centerPixelX,
                                  int* centerPixelY) {
  double centerX = lonToGlobalPixelX(lon, zoom);
  double centerY = latToGlobalPixelY(lat, zoom);
  long tileX = (long)floor(centerX / MAP_TILE_SIZE);
  long tileY = (long)floor(centerY / MAP_TILE_SIZE);
  int pixelX = (int)round(centerX);
  int pixelY = (int)round(centerY);
  if (centerTileX) *centerTileX = tileX;
  if (centerTileY) *centerTileY = tileY;
  if (centerPixelX) *centerPixelX = pixelX;
  if (centerPixelY) *centerPixelY = pixelY;
  return mapCanvasCached &&
         cachedMapZoom == zoom &&
         cachedMapTileX == tileX &&
         cachedMapTileY == tileY &&
         abs(cachedMapPixelX - pixelX) < 32 &&
         abs(cachedMapPixelY - pixelY) < 32 &&
         cachedMapHeight == plotH;
}

static bool mapRenderHasPendingResult() {
  return mapRenderResultPending ||
         (mapRenderResultQueue && uxQueueMessagesWaiting(mapRenderResultQueue) > 0);
}

static bool assembleMapTilesToBuffer(const MapRenderJob& job, MapRenderResult& result) {
  if (!mapWorkerBuf) {
    strlcpy(result.status, "render buffer missing", sizeof(result.status));
    return false;
  }

  SdLock lock(10000);
  if (!lock) {
    strlcpy(result.status, "SD busy", sizeof(result.status));
    return false;
  }
  if (!sdStorage.available || !mapTileRootFound) {
    strlcpy(result.status, "tiles unavailable", sizeof(result.status));
    return false;
  }

  char centerPath[96];
  buildTilePath(job.tileRoot, job.zoom, job.centerTileX, job.centerTileY, centerPath, sizeof(centerPath));
  result.centerTileFound = SD_MMC.exists(centerPath);
  if (!result.centerTileFound) {
    strlcpy(result.status, "center tile missing", sizeof(result.status));
    return false;
  }

  const lv_color_t bg = lv_color_make(0x07, 0x10, 0x0D);
  for (size_t i = 0; i < MAP_CANVAS_PIXELS; i++) {
    mapWorkerBuf[i] = bg;
  }

  int startX = job.centerPixelX - (MAP_PLOT_W / 2);
  int startY = job.centerPixelY - (job.plotH / 2);
  int endX = startX + MAP_PLOT_W - 1;
  int endY = startY + job.plotH - 1;
  long tileX0 = max(0L, (long)floor((double)startX / MAP_TILE_SIZE));
  long tileY0 = max(0L, (long)floor((double)startY / MAP_TILE_SIZE));
  long tileX1 = max(0L, (long)floor((double)endX / MAP_TILE_SIZE));
  long tileY1 = max(0L, (long)floor((double)endY / MAP_TILE_SIZE));
  uint16_t readBuf[MAP_PLOT_W];

  for (long tileY = tileY0; tileY <= tileY1; tileY++) {
    for (long tileX = tileX0; tileX <= tileX1; tileX++) {
      char path[96];
      buildTilePath(job.tileRoot, job.zoom, tileX, tileY, path, sizeof(path));
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
        size_t bytesRead = tile.read((uint8_t*)readBuf, bytesToRead);
        int pixelsRead = bytesRead / sizeof(uint16_t);
        for (int i = 0; i < pixelsRead; i++) {
          mapWorkerBuf[outY * MAP_PLOT_W + outX + i] = rgb565ToLvColor(readBuf[i]);
        }
        if ((globalY & 0x0F) == 0) delay(0);
      }
      tile.close();
      delay(0);
    }
  }

  result.drewTiles = true;
  strlcpy(result.status, "drawn", sizeof(result.status));
  return true;
}

static void mapRenderTask(void*) {
  MapRenderJob job = mapRenderActiveJob;
  MapRenderResult result = {};
  result.id = job.id;
  result.lat = job.lat;
  result.lon = job.lon;
  result.zoom = job.zoom;
  result.plotH = job.plotH;
  result.centerTileX = job.centerTileX;
  result.centerTileY = job.centerTileY;
  result.centerPixelX = job.centerPixelX;
  result.centerPixelY = job.centerPixelY;
  uint32_t startedMs = millis();
  assembleMapTilesToBuffer(job, result);
  result.durationMs = millis() - startedMs;
  if (mapRenderResultQueue) {
    xQueueOverwrite(mapRenderResultQueue, &result);
    mapRenderResultPending = true;
  } else {
    mapRenderWorkerFailures++;
  }
  mapRenderWorkerRunning = false;
  mapRenderTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

static bool requestMapTileRender(double lat,
                                 double lon,
                                 int zoom,
                                 uint16_t plotH,
                                 long centerTileX,
                                 long centerTileY,
                                 int centerPixelX,
                                 int centerPixelY) {
  if (!mapWorkerBuf || !mapRenderResultQueue) {
    mapRenderWorkerFailures++;
    strlcpy(mapCacheStatus, "worker unavailable", sizeof(mapCacheStatus));
    return false;
  }
  if (mapRenderWorkerRunning || mapRenderHasPendingResult()) {
    if (!mapRenderJobMatches(mapRenderActiveJob, lat, lon, zoom, plotH, centerTileX, centerTileY, centerPixelX, centerPixelY)) {
      mapRenderRetryPending = true;
    }
    mapRenderWorkerBusySkips++;
    strlcpy(mapCacheStatus, mapRenderResultPending ? "map ready" : "drawing...", sizeof(mapCacheStatus));
    return true;
  }

  MapRenderJob job = {};
  job.id = nextMapRenderJobId++;
  if (nextMapRenderJobId == 0) nextMapRenderJobId = 1;
  job.lat = lat;
  job.lon = lon;
  job.zoom = zoom;
  job.plotH = plotH;
  job.centerTileX = centerTileX;
  job.centerTileY = centerTileY;
  job.centerPixelX = centerPixelX;
  job.centerPixelY = centerPixelY;
  strlcpy(job.tileRoot, mapTileRoot, sizeof(job.tileRoot));
  mapRenderActiveJob = job;
  mapRenderWorkerRunning = true;
  mapRenderWorkerStarts++;
  strlcpy(mapCacheStatus, "drawing...", sizeof(mapCacheStatus));
  BaseType_t ok = xTaskCreatePinnedToCore(mapRenderTask, "mapRender", 8192, nullptr, 1, &mapRenderTaskHandle, 0);
  if (ok != pdPASS) {
    mapRenderWorkerRunning = false;
    mapRenderTaskHandle = nullptr;
    mapRenderWorkerFailures++;
    strlcpy(mapCacheStatus, "render task failed", sizeof(mapCacheStatus));
    return false;
  }
  return true;
}

static void serviceMapRenderWorker() {
  if (!mapRenderResultQueue || !mapRenderHasPendingResult()) return;
  MapRenderResult result = {};
  if (xQueueReceive(mapRenderResultQueue, &result, 0) != pdTRUE) return;
  mapRenderResultPending = false;
  mapRenderWorkerApplies++;

  if (result.centerTileFound && result.drewTiles && mapCanvas && mapCanvasBuf && mapWorkerBuf) {
    memcpy(mapCanvasBuf, mapWorkerBuf, MAP_CANVAS_BYTES);
    cachedMapZoom = result.zoom;
    cachedMapTileX = result.centerTileX;
    cachedMapTileY = result.centerTileY;
    cachedMapPixelX = result.centerPixelX;
    cachedMapPixelY = result.centerPixelY;
    cachedMapHeight = result.plotH;
    cachedMapLat = result.lat;
    cachedMapLon = result.lon;
    cachedMapTileFound = true;
    mapCanvasCached = true;
    strlcpy(mapCacheStatus, result.status[0] ? result.status : "drawn", sizeof(mapCacheStatus));
    lv_obj_invalidate(mapCanvas);
    recordMapRenderTimingDuration(result.durationMs, true, false);
    if (!mapNodeDetailViewActive && (!lastMapCacheSaveMs || millis() - lastMapCacheSaveMs > 15000)) {
      saveMapCacheToSd();
      lastMapCacheSaveMs = millis();
    }
  } else {
    cachedMapZoom = result.zoom;
    cachedMapTileX = result.centerTileX;
    cachedMapTileY = result.centerTileY;
    cachedMapLat = result.lat;
    cachedMapLon = result.lon;
    cachedMapTileFound = false;
    if (!mapCanvasCached && mapCanvas) {
      lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
      lv_obj_invalidate(mapCanvas);
    }
    strlcpy(mapCacheStatus, result.status[0] ? result.status : "render failed", sizeof(mapCacheStatus));
    recordMapRenderTimingDuration(result.durationMs, false, false);
  }

  if (mapRenderRetryPending) {
    mapRenderRetryPending = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
  }
  forcePageRefresh = true;
}

static bool renderOfflineTileMap(double lat, double lon, int zoom, char* centerPath, size_t centerPathSize) {
  uint32_t renderStartedMs = millis();
  if (!mapCanvas || !sdStorage.available || !mapTileRootFound) {
    recordMapRenderTiming(renderStartedMs, false, false);
    return false;
  }
  int plotH = activeMapPlotHeight();

  double centerX = lonToGlobalPixelX(lon, zoom);
  double centerY = latToGlobalPixelY(lat, zoom);
  long centerTileX = (long)floor(centerX / MAP_TILE_SIZE);
  long centerTileY = (long)floor(centerY / MAP_TILE_SIZE);
  int centerPixelX = (int)round(centerX);
  int centerPixelY = (int)round(centerY);
  if (mapCacheMatchesCenter(lat, lon, zoom, (uint16_t)plotH, &centerTileX, &centerTileY, &centerPixelX, &centerPixelY)) {
    recordMapRenderTiming(renderStartedMs, false, true);
    return cachedMapTileFound;
  }

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
    recordMapRenderTiming(renderStartedMs, false, false);
    return false;
  }

  requestMapTileRender(lat, lon, zoom, (uint16_t)plotH, centerTileX, centerTileY, centerPixelX, centerPixelY);
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

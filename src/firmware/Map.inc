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
      header.zoom < MAP_TILE_MIN_ZOOM || header.zoom > MAP_TILE_MAX_ZOOM ||
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

#include "TileCache.inc"
#include "MapCoverage.inc"

static bool assembleMapTilesToBuffer(const MapRenderJob& job, MapRenderResult& result, bool useTileCache) {
  result.drawnMask = 0;
  result.centerTileFound = result.drewTiles = false;
  if (!mapWorkerBuf || job.plotH == 0 || job.plotH > MAP_PLOT_H) {
    strlcpy(result.status, "render buffer/bounds invalid", sizeof(result.status));
    return false;
  }
  SdLock lock(10000);
  if (!lock) { strlcpy(result.status, "SD busy", sizeof(result.status)); return false; }
  if (job.storageGeneration != mapStorageGeneration.load() || !sdStorage.available || !mapTileRootFound) {
    strlcpy(result.status, "tile source changed/unavailable", sizeof(result.status));
    return false;
  }
  useTileCache = useTileCache && mapTileCacheBuffer;
  if (useTileCache) prepareMapTileCache(job);
  char centerPath[96];
  buildTilePath(job.tileRoot, job.zoom, job.centerTileX, job.centerTileY, centerPath, sizeof(centerPath));
  bool centerResident = useTileCache && mapTileIndex.find({job.zoom, (int32_t)job.centerTileX, (int32_t)job.centerTileY}, false) != mapTileIndex.missing;
  if (!centerResident && !SD_MMC.exists(centerPath)) {
    strlcpy(result.status, "center tile missing", sizeof(result.status));
    return false;
  }
  const lv_color_t bg = lv_color_make(0x07, 0x10, 0x0D);
  for (size_t i = 0; i < MAP_CANVAS_PIXELS; i++) mapWorkerBuf[i] = bg;
  int startX = job.centerPixelX - (MAP_PLOT_W / 2);
  int startY = job.centerPixelY - (job.plotH / 2);
  int endX = startX + MAP_PLOT_W - 1, endY = startY + job.plotH - 1;
  long tileX0 = max(0L, (long)floor((double)startX / MAP_TILE_SIZE));
  long tileY0 = max(0L, (long)floor((double)startY / MAP_TILE_SIZE));
  long tileX1 = min((1L << job.zoom) - 1, max(0L, (long)floor((double)endX / MAP_TILE_SIZE)));
  long tileY1 = min((1L << job.zoom) - 1, max(0L, (long)floor((double)endY / MAP_TILE_SIZE)));
  unsigned areaIndex = 0;
  uint16_t readBuf[MAP_PLOT_W];
  for (long tileY = tileY0; tileY <= tileY1; tileY++) {
    for (long tileX = tileX0; tileX <= tileX1; tileX++) {
      uint8_t areaBit = (uint8_t)(1U << areaIndex++);
      const uint16_t* cached = nullptr;
      File tile;
      if (useTileCache) {
        cached = loadCachedMapTile(job, tileX, tileY, result);
        if (!cached) continue;
      } else {
        char path[96];
        buildTilePath(job.tileRoot, job.zoom, tileX, tileY, path, sizeof(path));
        tile = SD_MMC.open(path, FILE_READ);
        if (!tile) continue;
        if (tile.isDirectory() || tile.size() != MAP_TILE_BYTES) {
          tile.close(); mapTileCacheReadErrors.fetch_add(1); continue;
        }
      }
      int tileGlobalX0 = tileX * MAP_TILE_SIZE, tileGlobalY0 = tileY * MAP_TILE_SIZE;
      int overlapX0 = max(startX, tileGlobalX0), overlapY0 = max(startY, tileGlobalY0);
      int overlapX1 = min(endX, tileGlobalX0 + MAP_TILE_SIZE - 1);
      int overlapY1 = min(endY, tileGlobalY0 + MAP_TILE_SIZE - 1);
      int segment = overlapX1 - overlapX0 + 1, outX = overlapX0 - startX, inTileX = overlapX0 - tileGlobalX0;
      for (int globalY = overlapY0; globalY <= overlapY1; globalY++) {
        int outY = globalY - startY, inTileY = globalY - tileGlobalY0;
        const uint16_t* pixels = cached ? cached + inTileY * MAP_TILE_SIZE + inTileX : readBuf;
        if (!cached) {
          uint32_t seekStarted = micros();
          bool sought = tile.seek((inTileY * MAP_TILE_SIZE + inTileX) * sizeof(uint16_t));
          result.tileReadUs += (uint32_t)(micros() - seekStarted);
          if (!sought || readMapTileBytes(tile, (uint8_t*)readBuf, segment * sizeof(uint16_t), result) != segment * sizeof(uint16_t)) {
            tile.close(); mapTileCacheReadErrors.fetch_add(1);
            result.centerTileFound = false;
            strlcpy(result.status, "tile read failed", sizeof(result.status));
            return false;
          }
        }
        for (int i = 0; i < segment; i++) mapWorkerBuf[outY * MAP_PLOT_W + outX + i] = rgb565ToLvColor(pixels[i]);
        if ((globalY & 0x0F) == 0) delay(0);
      }
      if (tile) tile.close();
      result.drawnMask |= areaBit;
      if (tileX == job.centerTileX && tileY == job.centerTileY) result.centerTileFound = true;
      delay(0);
    }
  }
  result.drewTiles = result.centerTileFound;
  strlcpy(result.status, result.drewTiles ? "drawn" : "center tile unreadable", sizeof(result.status));
  return result.drewTiles;
}

#include "MapBenchmark.inc"

static void mapRenderTask(void*) {
  MapRenderJob job = mapRenderActiveJob;
  MapRenderResult result = {};
  result.viewRevision = job.viewRevision;
  result.id = job.id;
  result.storageGeneration = job.storageGeneration;
  result.benchmark = job.benchmark;
  result.lat = job.lat;
  result.lon = job.lon;
  result.zoom = job.zoom;
  result.plotH = job.plotH;
  result.centerTileX = job.centerTileX;
  result.centerTileY = job.centerTileY;
  result.centerPixelX = job.centerPixelX;
  result.centerPixelY = job.centerPixelY;
  uint32_t startedMs = millis();
  if (job.benchmark) runMapBenchmark(job, result);
  else renderCoveredMap(job, result);
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
  job.viewRevision = mapView.revision;
  job.storageGeneration = mapStorageGeneration.load();
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
  if (result.benchmark) {
    memcpy(lastMapBenchmark, result.samples, sizeof(lastMapBenchmark));
    lastMapBenchmarkAvailable = true;
    lastMapBenchmarkPassed = result.benchmarkPassed;
    static const char* names[] = {"baseline", "cold", "warm", "baseline-pan", "cached-pan", "baseline-return", "cached-return"};
    for (size_t i = 0; i < 7; ++i) {
      const auto& sample = result.samples[i];
      Serial.printf("[mapbench] %s us=%lu tileReads=%lu tileBytes=%lu ok=%s pixels=%s\n", names[i],
                    (unsigned long)sample.durationUs, (unsigned long)sample.readCalls, (unsigned long)sample.readBytes,
                    sample.ok ? "true" : "false", sample.pixelsMatch ? "match" : "different");
    }
    Serial.printf("[mapbench] complete=%s status=%s\n", result.benchmarkPassed ? "PASS" : "FAIL", result.status);
    mapBenchmarkRunning = false;
    if (mapRenderRetryPending) {
      mapRenderRetryPending = false;
      mapRenderPending = true;
      lastMapUiRefreshMs = 0;
    }
    forcePageRefresh = true;
    return; // Benchmark pixels must never replace the displayed map or SD snapshot.
  }
  if (result.storageGeneration != mapStorageGeneration.load() || result.viewRevision != mapView.revision || mapDragging) {
    mapRenderRetryPending = false;
    mapRenderPending = true;
    lastMapUiRefreshMs = 0;
    forcePageRefresh = true;
    return;
  }
  lastMapTileReadUs = result.tileReadUs;
  mapCoverage = result.coverage;

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
    if (!mapNavigationTesting && !mapView.manual && mapView.follow && !mapNodeDetailViewActive && (!lastMapCacheSaveMs || millis() - lastMapCacheSaveMs > 15000)) {
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
    mapCanvasCached = false;
    if (mapCanvas) {
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
  lastMapUiRefreshMs = 0;
  forcePageRefresh = true;
}

static bool renderOfflineTileMap(double lat, double lon, int zoom, char* centerPath, size_t centerPathSize) {
  uint32_t started = millis();
  if (!mapCanvas || !sdStorage.available || !mapTileRootFound) return false;
  uint16_t height = (uint16_t)activeMapPlotHeight();
  bool checked = mapCoverageMatches(lat, lon, height);
  int actual = checked ? MapCoverage::choose(mapCoverage, zoom) : -1;
  if (checked && actual < 0) {
    mapCanvasCached = false;
    lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
    strlcpy(mapCacheStatus, "no usable coverage", sizeof(mapCacheStatus));
    return false;
  }
  if (checked && mapCacheMatchesCenter(lat, lon, actual, height)) {
    recordMapRenderTiming(started, false, true);
    return cachedMapTileFound;
  }
  double x = lonToGlobalPixelX(lon, zoom), y = latToGlobalPixelY(lat, zoom);
  long tileX = (long)floor(x / MAP_TILE_SIZE), tileY = (long)floor(y / MAP_TILE_SIZE);
  if (centerPath && centerPathSize) buildTilePath(mapTileRoot, zoom, tileX, tileY, centerPath, centerPathSize);
  requestMapTileRender(lat, lon, zoom, height, tileX, tileY, (int)round(x), (int)round(y));
  return true; // The worker resolves coverage; the main loop never probes SD files.
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

// Read-only comparison runs on the map worker; no benchmark image is displayed/saved.
static void runMapBenchmark(const MapRenderJob& original, MapRenderResult& result) {
  lv_color_t* reference = (lv_color_t*)heap_caps_malloc(MAP_CANVAS_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!reference || !mapTileCacheBuffer) {
    if (reference) heap_caps_free(reference);
    strlcpy(result.status, "PSRAM cache/reference unavailable", sizeof(result.status));
    return;
  }
  result.benchmarkPassed = true;
  const bool cached[] = {false, true, true, false, true, false, true};
  int localX = original.centerPixelX % MAP_TILE_SIZE;
  const int panOffset = localX > MAP_TILE_SIZE - 41 ? -40 : 40;
  for (size_t i = 0; i < 7; ++i) {
    MapRenderJob job = original;
    if (i == 3 || i == 4) {
      job.centerPixelX += panOffset;
      job.centerTileX = (long)floor((double)job.centerPixelX / MAP_TILE_SIZE);
    }
    if (i == 1) {
      SdLock lock(10000);
      if (!lock) { result.benchmarkPassed = false; break; }
      mapTileIndex.clear();
      mapTileCacheResident.store(0);
    }
    MapRenderResult rendered = {};
    uint32_t callsBefore = mapTileSdReadCalls.load(), bytesBefore = mapTileSdReadBytes.load();
    uint32_t start = micros();
    bool ok = assembleMapTilesToBuffer(job, rendered, cached[i]);
    MapBenchmarkSample& sample = result.samples[i];
    sample.durationUs = (uint32_t)(micros() - start);
    sample.readCalls = mapTileSdReadCalls.load() - callsBefore;
    sample.readBytes = mapTileSdReadBytes.load() - bytesBefore;
    sample.ok = ok;
    if (!cached[i] && ok) memcpy(reference, mapWorkerBuf, MAP_CANVAS_BYTES);
    sample.pixelsMatch = ok && memcmp(reference, mapWorkerBuf, MAP_CANVAS_BYTES) == 0;
    result.benchmarkPassed &= sample.ok && sample.pixelsMatch;
    // Repeated viewport and return must have no tile-data reads when cached.
    if (i == 2 || i == 6) result.benchmarkPassed &= sample.readCalls == 0;
    if (!ok) break;
    delay(1);
  }
  heap_caps_free(reference);
  strlcpy(result.status, result.benchmarkPassed ? "pixels identical; warm reads zero" : "render/pixel/cache check failed", sizeof(result.status));
}

static void requestMapBenchmark() {
  if (mapRenderWorkerRunning || mapRenderHasPendingResult() || mapBenchmarkRunning || usbTileUploadActive() || usbSdExportActive()) {
    Serial.println("[mapbench] busy; wait for map rendering or USB SD transfer");
    return;
  }
  if (!mapTileCacheBuffer || !mapWorkerBuf || !mapRenderResultQueue || !sdStorage.available || !mapTileRootFound) {
    Serial.println("[mapbench] unavailable: need PSRAM cache and SD tiles");
    return;
  }
  if (!mapCanvasCached && !lastMapLocationValid && !gpsStats.valid) {
    Serial.println("[mapbench] unavailable: open Map with a valid location first");
    return;
  }
  MapRenderJob job = {};
  job.benchmark = true;
  job.lat = mapCanvasCached ? cachedMapLat : (gpsStats.valid ? gpsStats.latitude : defaultMapLat);
  job.lon = mapCanvasCached ? cachedMapLon : (gpsStats.valid ? gpsStats.longitude : defaultMapLon);
  char path[96];
  job.zoom = findBestMapZoom(job.lat, job.lon, path, sizeof(path));
  job.storageGeneration = mapStorageGeneration.load();
  job.id = nextMapRenderJobId++;
  if (nextMapRenderJobId == 0) nextMapRenderJobId = 1;
  job.plotH = activeMapPlotHeight();
  job.centerPixelX = (int)round(lonToGlobalPixelX(job.lon, job.zoom));
  job.centerPixelY = (int)round(latToGlobalPixelY(job.lat, job.zoom));
  job.centerTileX = (long)floor((double)job.centerPixelX / MAP_TILE_SIZE);
  job.centerTileY = (long)floor((double)job.centerPixelY / MAP_TILE_SIZE);
  strlcpy(job.tileRoot, mapTileRoot, sizeof(job.tileRoot));
  mapRenderActiveJob = job;
  mapBenchmarkRunning = true;
  mapRenderWorkerRunning = true;
  mapRenderWorkerStarts++;
  if (xTaskCreatePinnedToCore(mapRenderTask, "mapBench", 8192, nullptr, 1, &mapRenderTaskHandle, 0) != pdPASS) {
    mapBenchmarkRunning = false;
    mapRenderWorkerRunning = false;
    mapRenderTaskHandle = nullptr;
    mapRenderWorkerFailures++;
    Serial.println("[mapbench] task allocation failed");
  } else {
    Serial.println("[mapbench] started: uncached, cold, warm, pan and return");
  }
}

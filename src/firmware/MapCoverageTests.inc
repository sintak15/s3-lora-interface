#ifdef FIRMWARE_RUN_TESTS
// Use a uniquely named fixture tree, never alter live tiles or location history.
static void runMapCoverageTests() {
  if (!sdStorage.available || mapRenderWorkerRunning || mapRenderHasPendingResult() || mapDragging || usbTileUploadActive() || usbSdExportActive()) {
    Serial.println("[test-coverage] busy/unavailable; retry after rendering/transfers"); return;
  }
  auto savedCoverage = mapCoverage;
  auto savedView = mapView;
  auto savedMode = mapCenterMode;
  bool savedDetail = mapNodeDetailViewActive;
  lv_obj_t* savedPage = currentPage;
  bool all = true;
  auto check = [&all](const char* name, bool ok) { Serial.printf("[test-coverage] %s: %s\n", name, ok ? "PASS" : "FAIL"); all &= ok; };
  char (*paths)[112] = (char (*)[112])heap_caps_malloc(20 * 112, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t* data = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!paths || !data) { if (paths) heap_caps_free(paths); if (data) heap_caps_free(data); check("Fixture allocation", false); return; }
  memset(data, 0x36, 4096);
  unsigned created = 0;
  MapRenderJob job = {};
  job.storageGeneration = mapStorageGeneration.load();
  job.viewRevision = mapView.revision;
  job.lat = 44.9537; job.lon = -93.09;
  job.zoom = 14;
  job.plotH = (uint16_t)activeMapPlotHeight();
  do { snprintf(job.tileRoot, sizeof(job.tileRoot), "/s3-lora/test-coverage-%08lx", (unsigned long)esp_random()); } while (SD_MMC.exists(job.tileRoot));
  auto writeTile = [&](int z, MapCoverage::Cell cell, bool valid) {
    SdLock lock;
    if (!lock || created == 20) return false;
    char zoomPath[96], xPath[112];
    buildTileRootPath(job.tileRoot, z, zoomPath, sizeof(zoomPath));
    snprintf(xPath, sizeof(xPath), "%s/%ld", zoomPath, (long)cell.x);
    if ((!SD_MMC.exists(job.tileRoot) && !SD_MMC.mkdir(job.tileRoot)) ||
        (!SD_MMC.exists(zoomPath) && !SD_MMC.mkdir(zoomPath)) || (!SD_MMC.exists(xPath) && !SD_MMC.mkdir(xPath))) return false;
    buildTilePath(job.tileRoot, z, cell.x, cell.y, paths[created], 112);
    File file = SD_MMC.open(paths[created++], FILE_WRITE);
    if (!file) return false;
    bool ok = true;
    for (size_t i = 0; i < (valid ? MAP_TILE_BYTES : 8); i += 4096) {
      size_t n = valid ? 4096 : 8;
      if (file.write(data, n) != n) { ok = false; break; }
      delay(0);
    }
    file.close();
    return ok;
  };
  auto area = [&](int z) { return MapCoverage::footprint(lonToGlobalPixelX(job.lon, z), latToGlobalPixelY(job.lat, z), z, MAP_PLOT_W, job.plotH); };
  auto centerCell = [&](int z) {
    auto a = area(z);
    for (unsigned i = 0; i < a.count; ++i) if (a.centerMask & (1U << i)) return a.cells[i];
    return MapCoverage::Cell{};
  };
  bool files = writeTile(14, centerCell(14), true) && writeTile(13, centerCell(13), false);
  auto full = area(12);
  for (unsigned i = 0; i < full.count; ++i) files &= writeTile(12, full.cells[i], true);
  check("Fixture files created", files);
  if (files) {
    auto profile = probeMapCoverage(job);
    check("Full/partial/malformed/missing detection", profile.ready && profile.zooms[2].complete() && profile.zooms[4].center() && !profile.zooms[4].complete() && profile.zooms[3].invalid() == 1 && !profile.zooms[1].available());
    check("Complete coarser viewport preferred", MapCoverage::choose(profile, 14) == 12);
    check("Never zoom above requested level", MapCoverage::choose(profile, 11) == -1);
    MapRenderResult rendered = {};
    renderCoveredMap(job, rendered);
    check("Worker renders coarser fallback", rendered.drewTiles && rendered.centerTileFound && rendered.zoom == 12 && rendered.coverage.zooms[2].complete());

    mapNavigationTesting = true;
    mapCoverage = rendered.coverage;
    mapView.zoom = 14; mapView.center = {job.lat, job.lon}; mapView.hasCenter = mapView.manual = true; mapView.follow = false;
    mapCenterMode = MAP_CENTER_CURRENT_DEVICE; mapNodeDetailViewActive = false;
    showPage(pageGps, false);
    // Only state/labels are exercised; fixture pixels never enter mapCanvasBuf.
    mapCanvasCached = true; cachedMapZoom = 12;
    updateMapControls(); updateMapCoverageUi();
    check("Requested and displayed levels remain distinct", strstr(lv_label_get_text(lblMapZoom), "14") && strstr(lv_label_get_text(lblMapCoverage), "View 12") && strstr(lv_label_get_text(lblMapCoverage), "Coarser"));
    lv_event_send(btnMapCoverage, LV_EVENT_CLICKED, nullptr);
    lv_obj_update_layout(pageMapCoverage);
    bool layout = currentPage == pageMapCoverage;
    lv_area_t pageArea; lv_obj_get_coords(pageMapCoverage, &pageArea);
    for (int i = 0; i < MapCoverage::levels; ++i) {
      lv_area_t row; lv_obj_get_coords(lv_obj_get_parent(lblMapCoverageRows[i]), &row);
      layout &= row.x1 >= pageArea.x1 && row.x2 <= pageArea.x2 && row.y1 >= pageArea.y1 && row.y2 <= pageArea.y2;
    }
    check("Coverage page and malformed-file status", layout && strstr(lv_label_get_text(lblMapCoverageRows[3]), "Unreadable"));
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr);
    check("Coverage Back returns to map", currentPage == pageGps);
    mapNavigationTesting = false;

    // New detailed files become available without changing the requested zoom.
    auto detailed = area(14);
    bool added = true;
    for (unsigned i = 0; i < detailed.count; ++i) if (!(detailed.centerMask & (1U << i))) added &= writeTile(14, detailed.cells[i], true);
    MapRenderResult detail = {};
    renderCoveredMap(job, detail);
    check("Detailed coverage returns automatically", added && detail.zoom == 14 && detail.drewTiles && detail.coverage.zooms[4].complete());
    {
      SdLock lock;
      for (unsigned i = 0; lock && i < full.count; ++i) { char path[112]; buildTilePath(job.tileRoot, 12, full.cells[i].x, full.cells[i].y, path, sizeof(path)); SD_MMC.remove(path); }
      for (unsigned i = 0; lock && i < detailed.count; ++i) if (!(detailed.centerMask & (1U << i))) { char path[112]; buildTilePath(job.tileRoot, 14, detailed.cells[i].x, detailed.cells[i].y, path, sizeof(path)); SD_MMC.remove(path); }
    }
    MapRenderResult partial = {};
    renderCoveredMap(job, partial);
    check("Best partial view retained", partial.zoom == 14 && partial.drewTiles && !partial.coverage.zooms[4].complete() && partial.coverage.zooms[4].available() == 1);
    {
      SdLock lock;
      char path[112]; auto cell = centerCell(14); buildTilePath(job.tileRoot, 14, cell.x, cell.y, path, sizeof(path));
      if (lock) SD_MMC.remove(path);
    }
    auto empty = probeMapCoverage(job);
    check("No coverage is explicit", MapCoverage::choose(empty, 14) == -1);
    auto edge = MapCoverage::footprint(0, 0, 10, MAP_PLOT_W, job.plotH);
    auto corner = MapCoverage::footprint(256, 256, 10, MAP_PLOT_W, job.plotH);
    check("World edge and four-tile footprint", edge.count == 1 && edge.centerMask == 1 && corner.count == 4 && corner.centerMask == 8);
  }
  {
    SdLock lock;
    bool cleaned = (bool)lock;
    if (lock) {
      for (unsigned i = 0; i < created; ++i) if (SD_MMC.exists(paths[i])) cleaned &= SD_MMC.remove(paths[i]);
      for (unsigned i = 0; i < created; ++i) {
        char* slash = strrchr(paths[i], '/'); if (slash) { *slash = 0; SD_MMC.rmdir(paths[i]); }
        slash = strrchr(paths[i], '/'); if (slash) { *slash = 0; SD_MMC.rmdir(paths[i]); }
      }
      cleaned &= SD_MMC.rmdir(job.tileRoot);
    }
    check("Temporary fixture tree removed", cleaned);
  }
  heap_caps_free(paths); heap_caps_free(data);
  mapNavigationTesting = false;
  mapCoverage = savedCoverage; mapView = savedView; mapView.revision += 1000;
  mapCenterMode = savedMode; mapNodeDetailViewActive = savedDetail;
  invalidateMapView(); showPage(savedPage, false); updateMapControls(); updateMapCoverageUi();
  Serial.printf("[test-coverage] complete: %s\n", all ? "PASS" : "FAIL");
}
#endif

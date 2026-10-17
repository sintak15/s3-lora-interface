#ifdef FIRMWARE_RUN_TESTS
static lv_point_t testMapTouchPoint;
static lv_indev_state_t testMapTouchState = LV_INDEV_STATE_REL;
static void readTestMapTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
  data->point = testMapTouchPoint;
  data->state = testMapTouchState;
}
// No fake positions are written to SD or preferences. Pending test renders are
// invalidated before normal operation resumes.
static void runMapNavigationTests() {
  if (mapRenderWorkerRunning || mapRenderHasPendingResult() || mapDragging || usbTileUploadActive() || usbSdExportActive()) {
    Serial.println("[test-map] busy; retry when transfers/rendering finish");
    return;
  }
  auto savedView = mapView;
  auto savedGps = gpsStats;
  auto savedMode = mapCenterMode;
  lv_obj_t* savedPage = currentPage;
  bool savedPending = mapZoomSavePending;
  uint32_t savedSaveAt = mapZoomSaveAtMs;
  bool savedNodeDetail = mapNodeDetailViewActive, savedFocus = mapFocusSelectedNode;
  bool savedLocationValid = lastMapLocationValid;
  double savedLat = defaultMapLat, savedLon = defaultMapLon;
  uint32_t savedOwn = stats.myNodeNum;
  bool all = true;
  auto check = [&all](const char* name, bool ok) { Serial.printf("[test-map] %s: %s\n", name, ok ? "PASS" : "FAIL"); all &= ok; };
  mapNavigationTesting = true;
  mapCenterMode = MAP_CENTER_CURRENT_DEVICE;
  mapNodeDetailViewActive = false;
  stats.myNodeNum = 0;
  gpsStats.valid = false;
  lastMapLocationValid = true;
  defaultMapLat = 44.9537; defaultMapLon = -93.09;
  mapView.resume();
  showPage(pageGps, false);
  refreshMapUi();
  lv_obj_update_layout(pageGps);
  lv_area_t area;
  lv_obj_get_coords(pageGps, &area);
  bool layout = !lv_obj_has_flag(pageGps, LV_OBJ_FLAG_SCROLLABLE);
  for (auto* object : {btnMapZoomOut, btnMapZoomIn, btnMapLocate, btnMapFollow, btnMapCoverage, mapPlot, lblMapStats}) {
    lv_area_t child;
    lv_obj_get_coords(object, &child);
    layout &= child.x1 >= area.x1 && child.x2 <= area.x2 && child.y1 >= area.y1 && child.y2 <= area.y2;
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE)) layout &= lv_obj_get_height(object) >= 44;
  }
  check("Map bounds and 44px controls", layout);
  for (int zoom = 10; zoom <= 14; ++zoom) {
    setMapZoom(zoom, false);
    char expected[24]; snprintf(expected, sizeof(expected), "Zoom %d\nCoverage", zoom);
    char name[16]; snprintf(name, sizeof(name), "Zoom %d", zoom);
    check(name, mapView.zoom == zoom && strcmp(lv_label_get_text(lblMapZoom), expected) == 0);
  }
  setMapZoom(99, false);
  check("Zoom upper bound", mapView.zoom == 14 && lv_obj_has_state(btnMapZoomIn, LV_STATE_DISABLED));
  setMapZoom(-5, false);
  check("Zoom lower bound", mapView.zoom == 10 && lv_obj_has_state(btnMapZoomOut, LV_STATE_DISABLED));

  auto origin = MapNavigation::Point{44.9537, -93.09};
  auto east = MapNavigation::pan(origin, 14, -80, 0);
  auto south = MapNavigation::pan(origin, 14, 0, -60);
  auto returned = MapNavigation::pan(east, 14, 80, 0);
  check("Drag direction and inverse", east.lon > origin.lon && south.lat < origin.lat && fabs(returned.lon-origin.lon) < 1e-9 && fabs(returned.lat-origin.lat) < 1e-9);
  auto edge = MapNavigation::pan({85.0, 179.999}, 10, -2000, 2000);
  check("Mercator limits and date line", edge.lat <= MapNavigation::maxLatitude && edge.lon >= -180 && edge.lon < 180);
  auto scale = MapNavigation::scale(origin.lat, 14);
  double metersPerPixel = cos(origin.lat * DEG_TO_RAD) * (2 * MapNavigation::pi * 6378137.0) / (256 * (1 << 14));
  check("Imperial scale distance and pixels", scale.miles == 0.25 && fabs(scale.meters - 402.336) < 0.001 && scale.pixels > 0 && scale.pixels <= 80 && fabs(scale.pixels*metersPerPixel - scale.meters) <= metersPerPixel);
  check("Feet scale at polar latitude", MapNavigation::scale(85, 14).feet == 200);

  gpsStats.valid = true;
  gpsStats.latitude = origin.lat; gpsStats.longitude = origin.lon;
  gpsStats.lastUpdateMs = millis();
  mapView.resume(); lastMapUiRefreshMs = 0; refreshMapUi();
  check("Live GPS source", strstr(lv_label_get_text(lblMapStats), "GPS live") && mapView.follow);
  // Drop the canvas cache so the test freezes the supplied view, not a boot snapshot.
  mapCanvasCached = false;
  setMapFollow(false);
  auto frozen = mapView.center;
  gpsStats.latitude += 0.1;
  lastMapUiRefreshMs = 0; refreshMapUi();
  check("Follow off freezes center", mapView.manual && !mapView.follow && mapView.center.lat == frozen.lat);
  lv_event_send(btnMapLocate, LV_EVENT_CLICKED, nullptr);
  check("My location resumes GPS", mapView.follow && !mapView.manual && mapView.center.lat == gpsStats.latitude);
  gpsStats.lastUpdateMs = millis() - 31000;
  lastMapUiRefreshMs = 0; refreshMapUi();
  check("Stale GPS uses saved location", strstr(lv_label_get_text(lblMapStats), "Saved location") && mapView.center.lat == defaultMapLat);
  mapView.panFrom(origin, 80, 60);
  check("Pan pauses following", mapView.manual && !mapView.follow && mapView.center.lon < origin.lon && mapView.center.lat > origin.lat);

  // Drive the actual LVGL pointer path through the plot and its non-clickable
  // canvas/scale children, rather than just calling the pan calculation.
  setMapZoom(14, false);
  mapView.center = origin; mapView.hasCenter = true; mapView.manual = false; mapView.follow = true;
  lv_obj_update_layout(pageGps);
  lv_area_t plotArea;
  lv_obj_get_coords(mapPlot, &plotArea);
  testMapTouchPoint = {(lv_coord_t)((plotArea.x1 + plotArea.x2) / 2), (lv_coord_t)((plotArea.y1 + plotArea.y2) / 2)};
  lv_indev_drv_t pointerDriver;
  lv_indev_drv_init(&pointerDriver);
  pointerDriver.type = LV_INDEV_TYPE_POINTER;
  pointerDriver.read_cb = readTestMapTouch;
  lv_indev_t* pointer = lv_indev_drv_register(&pointerDriver);
  bool gestureOk = pointer != nullptr;
  if (pointer) {
    testMapTouchState = LV_INDEV_STATE_PR;
    lv_indev_read_timer_cb(pointer->driver->read_timer);
    gestureOk &= mapDragging;
    check("LVGL press targets map", mapDragging);
    testMapTouchPoint.x += 60; testMapTouchPoint.y += 20;
    lv_indev_read_timer_cb(pointer->driver->read_timer);
    // Position changes are deferred until LVGL runs its layout pass.
    lv_obj_update_layout(mapLayer);
    check("LVGL drag preview moves image", mapDragMoved && lv_obj_get_x(mapLayer) == 60 && lv_obj_get_y(mapLayer) == 20);
    gestureOk &= mapDragMoved && lv_obj_get_x(mapLayer) == 60 && lv_obj_get_y(mapLayer) == 20;
    testMapTouchState = LV_INDEV_STATE_REL;
    lv_indev_read_timer_cb(pointer->driver->read_timer);
    auto expected = MapNavigation::pan(origin, 14, 60, 20);
    gestureOk &= !mapDragging && !mapView.follow && mapView.manual && fabs(mapView.center.lat - expected.lat) < 1e-9 && fabs(mapView.center.lon - expected.lon) < 1e-9;
    lv_indev_delete(pointer);
  }
  check("LVGL drag gesture pauses following", gestureOk);

  // Fabricate a completed old view: it must never publish after a navigation change.
  if (!mapRenderWorkerRunning && !mapRenderHasPendingResult()) {
    MapRenderResult old = {};
    old.storageGeneration = mapStorageGeneration.load();
    old.viewRevision = mapView.revision - 1;
    old.centerTileFound = old.drewTiles = true;
    mapCanvasCached = false;
    xQueueOverwrite(mapRenderResultQueue, &old);
    mapRenderResultPending = true;
    serviceMapRenderWorker();
    check("Old view result discarded", !mapCanvasCached && mapRenderPending);
  }
  mapView = savedView;
  // A test worker may still be finishing; choose a fresh revision it cannot match.
  mapView.revision += 1000;
  gpsStats = savedGps;
  mapCenterMode = savedMode;
  mapNodeDetailViewActive = savedNodeDetail;
  mapFocusSelectedNode = savedFocus;
  mapZoomSavePending = savedPending;
  mapZoomSaveAtMs = savedSaveAt;
  lastMapLocationValid = savedLocationValid;
  defaultMapLat = savedLat; defaultMapLon = savedLon;
  stats.myNodeNum = savedOwn;
  invalidateMapView();
  showPage(savedPage, false);
  mapNavigationTesting = false;
  updateMapControls();
  Serial.printf("[test-map] complete: %s\n", all ? "PASS" : "FAIL");
}
#endif

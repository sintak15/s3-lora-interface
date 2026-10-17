static bool mapDevicePosition(double& lat, double& lon, char* source, size_t sourceSize) {
  if (MapNavigation::fresh(gpsStats.valid, gpsStats.lastUpdateMs, millis())) {
    lat = gpsStats.latitude; lon = gpsStats.longitude;
    snprintf(source, sourceSize, "GPS live %lus", (unsigned long)((millis() - gpsStats.lastUpdateMs) / 1000));
    return true;
  }
  const NodeRecord* own = stats.myNodeNum ? findNode(stats.myNodeNum) : nullptr;
  if (own && own->hasPosition) {
    lat = own->latitude; lon = own->longitude;
    snprintf(source, sourceSize, "Device position %lum", (unsigned long)((millis() - own->lastPositionMs) / 60000));
    return true;
  }
  if (lastMapLocationValid) {
    lat = defaultMapLat; lon = defaultMapLon;
    strlcpy(source, "Saved location", sourceSize);
    return true;
  }
  if (gpsStats.valid) {
    lat = gpsStats.latitude; lon = gpsStats.longitude;
    snprintf(source, sourceSize, "GPS last %lum", (unsigned long)((millis() - gpsStats.lastUpdateMs) / 60000));
    return true;
  }
  strlcpy(source, "Waiting for location", sourceSize);
  return false;
}

#include "MapCoverageUi.inc"

static void refreshMapUi() {
  if (!mapPlot || !lblMapStats || (currentPage != pageGps && currentPage != pageMapCoverage) || mapDragging) return;
  uint32_t interval = mapRenderPending ? 120 : 1000;
  if (lastMapUiRefreshMs && millis() - lastMapUiRefreshMs < interval) return;
  lastMapUiRefreshMs = millis();
  mapRenderPending = false;
  updateMapControls();

  double lat = 0, lon = 0;
  char source[48];
  bool located = mapDevicePosition(lat, lon, source, sizeof(source));
  const NodeRecord* selected = mapCenterMode == MAP_CENTER_SELECTED_NODE ? findNode(selectedNodeNum) : nullptr;
  if (selected && selected->hasPosition) {
    if (!mapView.manual) { lat = selected->latitude; lon = selected->longitude; located = true; }
    snprintf(source, sizeof(source), "Node position %lum", (unsigned long)((millis() - selected->lastPositionMs) / 60000));
  }
  if (mapView.manual && mapView.hasCenter) {
    lat = mapView.center.lat; lon = mapView.center.lon; located = true;
  }
  mapView.hasCenter = located;
  if (located) {
    mapView.center = {lat, lon};
    if (!mapView.follow) mapView.manual = true;
  }
  char status[80];
  if (selected && !selected->hasPosition) strlcpy(status, "Selected node has no GPS", sizeof(status));
  else snprintf(status, sizeof(status), "%s | %s", source, mapView.follow ? "Following" : "Paused");
  lv_label_set_text(lblMapStats, status);
  for (auto* dot : mapDots) if (dot) lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

  const char* notice = nullptr;
  if (!located) {
    notice = "Waiting for GPS\nor a saved location";
    mapCanvasCached = false;
    if (mapCanvas) lv_canvas_fill_bg(mapCanvas, lv_color_hex(0x07100D), LV_OPA_COVER);
  } else if (!sdStorage.available || !mapTileRootFound) {
    notice = mapCanvasCached ? "Saved map\nSD tiles unavailable" : "SD map tiles unavailable";
  } else if (mapNavigationTesting) {
    notice = "Map navigation test";
  } else {
    char tilePath[96];
    bool exists = renderOfflineTileMap(lat, lon, mapView.zoom, tilePath, sizeof(tilePath));
    if (!exists) notice = "No usable maps here\nChoose another area";
    else if (!mapCanvasCached) notice = mapRenderWorkerRunning || mapRenderHasPendingResult() ? "Loading map..." : "Map could not be loaded";
  }

  // Overlay coordinates always use the displayed image, including small GPS moves
  // retained by the pixel cache. Never draw markers in a pending job's coordinates.
  if (mapCanvasCached) {
    for (size_t i = 0; i < nodeCount; ++i) {
      if (!nodes[i].hasPosition) continue;
      bool isSelected = selected && nodes[i].num == selected->num;
      lv_color_t color = lv_color_hex(nodes[i].num == stats.myNodeNum ? 0x00C985 : (isSelected ? 0xFFD166 : 0x68FFC0));
      placeMapDot(i, cachedMapLat, cachedMapLon, nodes[i].latitude, nodes[i].longitude, cachedMapZoom, color, isSelected ? 14 : 8);
    }
    if (MapNavigation::fresh(gpsStats.valid, gpsStats.lastUpdateMs, millis()))
      placeMapDot(MAP_DOT_COUNT - 1, cachedMapLat, cachedMapLon, gpsStats.latitude, gpsStats.longitude, cachedMapZoom, lv_color_hex(0x00C985), 10);
  }
  updateMapScale();
  updateMapCoverageUi();
  if (notice) {
    lv_label_set_text(lblMapNotice, notice);
    lv_obj_center(lblMapNotice);
    lv_obj_clear_flag(lblMapNotice, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(lblMapNotice);
  } else lv_obj_add_flag(lblMapNotice, LV_OBJ_FLAG_HIDDEN);
  clearUiDirty(UI_DIRTY_MAP);
}

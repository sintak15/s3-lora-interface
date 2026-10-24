static void updateMapCoverageUi() {
  if (!lblMapCoverage || !lblMapCoverageSummary) return;
  bool valid = sdStorage.available && mapTileRootFound && mapView.hasCenter && mapCoverageMatches(mapView.center.lat, mapView.center.lon, (uint16_t)activeMapPlotHeight());
  char text[112], shown[16];
  bool displayed = mapCanvasCached && cachedMapZoom >= MAP_TILE_MIN_ZOOM && cachedMapZoom <= MAP_TILE_MAX_ZOOM;
  if (displayed) snprintf(shown, sizeof(shown), "%d", cachedMapZoom);
  else strlcpy(shown, "--", sizeof(shown));
  snprintf(text, sizeof(text), "At this map center\nRequested %d | View %s", mapView.zoom, shown);
  setReadoutText(lblMapCoverageSummary, text);
  for (int i = 0; i < MapCoverage::levels; ++i) {
    const auto& level = mapCoverage.zooms[i];
    if (valid) snprintf(text, sizeof(text), "Zoom %d: %s (%u/%u)", i + MAP_TILE_MIN_ZOOM, MapCoverage::status(level), level.available(), (unsigned)level.area.count);
    else snprintf(text, sizeof(text), "Zoom %d: %s", i + MAP_TILE_MIN_ZOOM, mapView.hasCenter && sdStorage.available ? "Checking" : "Not checked");
    setReadoutText(lblMapCoverageRows[i], text);
  }
  if (!mapView.hasCenter) strlcpy(text, "Coverage: location needed", sizeof(text));
  else if (!sdStorage.available || !mapTileRootFound) strlcpy(text, "Coverage: SD unavailable", sizeof(text));
  else if (!valid) snprintf(text, sizeof(text), "View %s | Checking area", shown);
  else if (!displayed) strlcpy(text, MapCoverage::choose(mapCoverage, mapView.zoom) < 0 ? "No map coverage here" : "Loading available map", sizeof(text));
  else {
    const auto& actual = mapCoverage.zooms[cachedMapZoom - MAP_TILE_MIN_ZOOM];
    if (!actual.complete()) snprintf(text, sizeof(text), "View %d | Partial %u/%u", cachedMapZoom, actual.available(), (unsigned)actual.area.count);
    else snprintf(text, sizeof(text), "View %d | %s", cachedMapZoom, cachedMapZoom < mapView.zoom ? "Coarser map" : "Full area");
  }
  setReadoutText(lblMapCoverage, text);
  lv_obj_move_foreground(lblMapCoverage);
}

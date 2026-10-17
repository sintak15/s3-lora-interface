// Raw RGB565 tiles are decoded in the browser. Snapshot to PSRAM before sending
// so network backpressure never holds the SD mutex or the map-worker buffers.
struct WebTileResult {
  int code = 404, zoom = -1;
  uint32_t generation = 0;
};

static WebTileResult readWebMapTile(const char* root, uint32_t z, uint32_t x, uint32_t y, uint8_t* pixels) {
  WebTileResult result;
  if (!TileUpload::validCoordinates(z, x, y) || !pixels) { result.code = 400; return result; }
  SdLock lock(0);
  if (!lock || !sdStorage.available) { result.code = 503; return result; }
  result.generation = mapStorageGeneration.load();
  bool unreadable = false;
  for (int level = (int)z; level >= MAP_TILE_MIN_ZOOM; --level) {
    int shift = (int)z - level;
    char path[112];
    buildTilePath(root, level, x >> shift, y >> shift, path, sizeof(path));
    File file = SD_MMC.open(path, FILE_READ);
    if (!file) { unreadable |= SD_MMC.exists(path); continue; }
    if (file.isDirectory() || file.size() != MAP_TILE_BYTES) { file.close(); unreadable = true; continue; }
    bool complete = true;
    for (size_t offset = 0; offset < MAP_TILE_BYTES; offset += 4096) {
      if (file.read(pixels + offset, 4096) != 4096) { complete = false; break; }
      delay(0);
    }
    file.close();
    if (!complete) { unreadable = true; continue; }
    result.code = 200; result.zoom = level;
    return result;
  }
  result.code = unreadable ? 422 : 404;
  return result;
}

static void handleWebMapTile() {
  if (!requireWebAuth()) return;
  server.sendHeader("Cache-Control", "no-store");
  uint32_t z, x, y;
  if (!tileNumber(server.arg("z").c_str(), z) || !tileNumber(server.arg("x").c_str(), x) ||
      !tileNumber(server.arg("y").c_str(), y) || !TileUpload::validCoordinates(z, x, y)) {
    server.send(400, "text/plain", "invalid tile coordinates"); return;
  }
  if (sdDownload.active || usbTileUploadActive() || usbSdExportActive() || mapBenchmarkRunning) {
    server.sendHeader("Retry-After", "1");
    server.send(503, "text/plain", "map storage busy"); return;
  }
  if (!sdStorage.available || !mapTileRootFound) { server.send(503, "text/plain", "SD maps unavailable"); return; }
  uint8_t* pixels = (uint8_t*)heap_caps_malloc(MAP_TILE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!pixels) { server.send(503, "text/plain", "map buffer unavailable"); return; }
  auto result = readWebMapTile(mapTileRoot, z, x, y, pixels);
  if (result.code != 200) {
    heap_caps_free(pixels);
    server.send(result.code, "text/plain", result.code == 404 ? "no map coverage" : (result.code == 422 ? "map files unreadable" : "SD busy"));
    return;
  }
  sdDownload.memory = pixels;
  sdDownload.memoryOffset = sdDownload.buffered = sdDownload.offset = 0;
  sdDownload.remaining = MAP_TILE_BYTES;
  sdDownload.path = nullptr;
  sdDownload.lastProgressMs = millis();
  sdDownload.client = server.client();
  sdDownload.active = true;
  server.sendHeader("X-Tile-Zoom", String(result.zoom));
  server.sendHeader("X-Map-Generation", String(result.generation));
  server.setContentLength(MAP_TILE_BYTES);
  server.send(200, "application/octet-stream", "");
}

static void registerWebMapRoutes() {
  server.on("/map/tile", HTTP_GET, handleWebMapTile);
  server.on("/assets/leaflet-1.9.4.js", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    server.sendHeader("Cache-Control", "private, max-age=86400");
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "application/javascript", (const char*)leaflet_js_gz, sizeof(leaflet_js_gz));
  });
  server.on("/assets/leaflet-1.9.4.css", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    server.sendHeader("Cache-Control", "private, max-age=86400");
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/css", (const char*)leaflet_css_gz, sizeof(leaflet_css_gz));
  });
}

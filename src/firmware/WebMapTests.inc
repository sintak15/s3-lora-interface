#ifdef FIRMWARE_RUN_TESTS
static std::atomic<bool> webMapTestsRunning{false};
static void webMapTestsTask(void*) {
  bool all = true;
  auto check = [&all](const char* name, bool ok) { Serial.printf("[test-webmap] %s: %s\n", name, ok ? "PASS" : "FAIL"); all &= ok; };
  uint8_t* pixels = (uint8_t*)heap_caps_malloc(MAP_TILE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!pixels) { check("PSRAM allocation", false); webMapTestsRunning = false; vTaskDelete(nullptr); return; }
  char root[64]; snprintf(root, sizeof(root), "/s3-lora/test-webmap-%08lx", (unsigned long)esp_random());
  char z12[80], x12[96], parent[112], z14[80], x14[96], detail[112];
  buildTileRootPath(root, 12, z12, sizeof(z12)); snprintf(x12, sizeof(x12), "%s/988", z12); buildTilePath(root, 12, 988, 1474, parent, sizeof(parent));
  buildTileRootPath(root, 14, z14, sizeof(z14)); snprintf(x14, sizeof(x14), "%s/3955", z14); buildTilePath(root, 14, 3955, 5898, detail, sizeof(detail));
  bool fixtures = false;
  {
    SdLock lock;
    if (lock && !SD_MMC.exists(root) && SD_MMC.mkdir(root) && SD_MMC.mkdir(z12) && SD_MMC.mkdir(x12) && SD_MMC.mkdir(z14) && SD_MMC.mkdir(x14)) {
      memset(pixels, 0x36, MAP_TILE_BYTES);
      File file = SD_MMC.open(parent, FILE_WRITE);
      fixtures = file && file.write(pixels, MAP_TILE_BYTES) == MAP_TILE_BYTES; if (file) file.close();
      File bad = SD_MMC.open(detail, FILE_WRITE);
      fixtures &= bad && bad.write(pixels, 8) == 8; if (bad) bad.close();
    }
  }
  check("Temporary fixture creation", fixtures);
  if (fixtures) {
    auto fallback = readWebMapTile(root, 14, 3955, 5898, pixels);
    check("Malformed detail falls back to parent", fallback.code == 200 && fallback.zoom == 12 && pixels[0] == 0x36 && pixels[MAP_TILE_BYTES-1] == 0x36);
    {
      SdLock lock;
      if (lock) { SD_MMC.remove(detail); File file = SD_MMC.open(detail, FILE_WRITE); if (file) { file.write(pixels, MAP_TILE_BYTES); file.close(); } }
    }
    auto exact = readWebMapTile(root, 14, 3955, 5898, pixels);
    check("Detailed tile recovery", exact.code == 200 && exact.zoom == 14);
    auto missing = readWebMapTile(root, 10, 0, 0, pixels);
    check("Missing coverage returns 404", missing.code == 404);
    auto invalid = readWebMapTile(root, 32, 0, 0, pixels);
    check("Invalid zoom rejected before shifting", invalid.code == 400);
  }
  {
    SdLock lock; bool cleaned = (bool)lock;
    if (lock) {
      if (SD_MMC.exists(parent)) cleaned &= SD_MMC.remove(parent);
      if (SD_MMC.exists(detail)) cleaned &= SD_MMC.remove(detail);
      if (SD_MMC.exists(x12)) cleaned &= SD_MMC.rmdir(x12);
      if (SD_MMC.exists(x14)) cleaned &= SD_MMC.rmdir(x14);
      if (SD_MMC.exists(z12)) cleaned &= SD_MMC.rmdir(z12);
      if (SD_MMC.exists(z14)) cleaned &= SD_MMC.rmdir(z14);
      if (SD_MMC.exists(root)) cleaned &= SD_MMC.rmdir(root);
    }
    check("Temporary fixtures removed", cleaned);
  }
  int z = 14;
  double lat = lastMapLocationValid ? defaultMapLat : 44.9537, lon = lastMapLocationValid ? defaultMapLon : -93.09;
  uint32_t x = (uint32_t)floor(lonToGlobalPixelX(lon, z) / 256), y = (uint32_t)floor(latToGlobalPixelY(lat, z) / 256);
  auto original = readWebMapTile(mapTileRoot, z, x, y, pixels);
  uint32_t expectedCrc = TileUpload::crcUpdate(0xffffffffU, pixels, MAP_TILE_BYTES) ^ 0xffffffffU;
  heap_caps_free(pixels);
  // Request the real authenticated routes through lwIP while the main loop runs
  // WebServer and services the nonblocking response stream.
  String credential = String(webUiUser) + ":" + webUiPass;
  unsigned char encoded[256]; size_t encodedBytes = 0;
  int encodedOk = mbedtls_base64_encode(encoded, sizeof(encoded)-1, &encodedBytes, (const unsigned char*)credential.c_str(), credential.length());
  encoded[encodedBytes] = 0;
  auto request = [&](const char* route, int expectedCode, size_t expectedBytes, uint32_t crc, int tileZoom, bool auth) {
    NetworkClient client; client.setTimeout(5000);
    if (encodedOk || !client.connect(IPAddress(127,0,0,1), 80, 3000)) return false;
    client.printf("GET %s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n", route);
    if (auth) client.printf("Authorization: Basic %s\r\n", encoded);
    client.print("\r\n");
    String status = client.readStringUntil('\n');
    int code = status.substring(9,12).toInt();
    size_t length = 0; int actualZoom = -1;
    uint32_t started = millis();
    while (client.connected() && millis() - started < 10000) {
      String header = client.readStringUntil('\n'); header.trim();
      if (!header.length()) break;
      if (header.startsWith("Content-Length:")) length = header.substring(15).toInt();
      if (header.startsWith("X-Tile-Zoom:")) actualZoom = header.substring(12).toInt();
    }
    uint8_t buffer[1024]; size_t received = 0; uint32_t actualCrc = 0xffffffffU;
    while (received < length && millis() - started < 15000) {
      int n = client.read(buffer, min(sizeof(buffer), length - received));
      if (n > 0) { actualCrc = TileUpload::crcUpdate(actualCrc, buffer, n); received += n; }
      else if (!client.connected()) break;
      else delay(1);
    }
    client.stop(); delay(30);
    return code == expectedCode && received == length && (expectedBytes == 0 || length == expectedBytes) && (crc == 0 || (actualCrc ^ 0xffffffffU) == crc) && (tileZoom == -1 || actualZoom == tileZoom);
  };
  check("HTTP local JavaScript exact bytes", request("/assets/leaflet-1.9.4.js", 200, sizeof(leaflet_js_gz), TileUpload::crcUpdate(0xffffffffU, leaflet_js_gz, sizeof(leaflet_js_gz)) ^ 0xffffffffU, -1, true));
  check("HTTP local stylesheet exact bytes", request("/assets/leaflet-1.9.4.css", 200, sizeof(leaflet_css_gz), TileUpload::crcUpdate(0xffffffffU, leaflet_css_gz, sizeof(leaflet_css_gz)) ^ 0xffffffffU, -1, true));
  char route[112]; snprintf(route, sizeof(route), "/map/tile?z=%d&x=%lu&y=%lu", z, (unsigned long)x, (unsigned long)y);
  check("HTTP tile matches SD CRC", original.code == 200 && request(route, 200, MAP_TILE_BYTES, expectedCrc, original.zoom, true));
  {
    NetworkClient slow; slow.setTimeout(3000);
    bool opened = slow.connect(IPAddress(127,0,0,1), 80, 3000);
    if (opened) {
      slow.printf("GET %s HTTP/1.1\r\nHost: localhost\r\nAuthorization: Basic %s\r\nConnection: close\r\n\r\n", route, encoded);
      String status = slow.readStringUntil('\n');
      opened &= status.startsWith("HTTP/1.1 200");
      while (slow.connected()) { String header = slow.readStringUntil('\n'); header.trim(); if (!header.length()) break; }
    }
    check("HTTP status remains responsive during tile send", opened && request("/status", 200, 0, 0, -1, true));
    slow.stop(); delay(150);
    check("HTTP tile buffer recovers after disconnect", request(route, 200, MAP_TILE_BYTES, expectedCrc, original.zoom, true));
  }
  check("HTTP tile requires authentication", request(route, 401, 0, 0, -1, false));
  check("HTTP invalid zoom rejected", request("/map/tile?z=32&x=0&y=0", 400, 0, 0, -1, true));
  check("HTTP out-of-world coordinate rejected", request("/map/tile?z=10&x=1024&y=0", 400, 0, 0, -1, true));
  check("HTTP path traversal rejected", request("/map/tile?z=14&x=..%2F..&y=0", 400, 0, 0, -1, true));
  Serial.printf("[test-webmap] complete: %s\n", all ? "PASS" : "FAIL");
  webMapTestsRunning = false;
  vTaskDelete(nullptr);
}

static void runWebMapTests() {
  if (webMapTestsRunning || !wifiEnabled || !sdStorage.available || sdDownload.active || mapRenderWorkerRunning || usbTileUploadActive() || usbSdExportActive()) {
    Serial.println("[test-webmap] busy/unavailable; retry when WiFi and SD are available"); return;
  }
  webMapTestsRunning = true;
  if (xTaskCreatePinnedToCore(webMapTestsTask, "testWebMap", 12288, nullptr, 1, nullptr, 0) != pdPASS) {
    webMapTestsRunning = false; Serial.println("[test-webmap] task allocation: FAIL");
  }
}
#endif

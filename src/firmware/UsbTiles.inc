// USB-only tile protocol. The radio UART and web serial console cannot write tiles.
// A complete tile is staged in PSRAM; its CRC and the SD readback must agree before promotion.
static uint8_t* usbTileBuffer = nullptr;
static uint32_t usbTileId = 0, usbTileExpectedCrc = 0, usbTileLastMs = 0;
static size_t usbTileReceived = 0;
static char usbTilePath[128] = "";

static bool usbTileUploadActive() { return usbTileBuffer != nullptr; }

static void resetUsbTileUpload() {
  if (usbTileBuffer) heap_caps_free(usbTileBuffer);
  usbTileBuffer = nullptr;
  usbTileId = 0;
  usbTileReceived = 0;
}

static void serviceUsbTileTimeout() {
  if (usbTileUploadActive() && (uint32_t)(millis() - usbTileLastMs) > 30000) {
    Serial.printf("[TILEUSB] %08lx ERROR timeout\n", (unsigned long)usbTileId);
    resetUsbTileUpload();
  }
}

static bool tileNumber(const char* text, uint32_t& value, int base = 10) {
  if (!text || !*text || strlen(text) > (base == 16 ? 8U : 10U)) return false;
  for (const char* p = text; *p; ++p) {
    if (!((*p >= '0' && *p <= '9') || (base == 16 && ((*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))))) return false;
  }
  errno = 0;
  char* end = nullptr;
  unsigned long parsed = strtoul(text, &end, base);
  if (errno || *end || parsed > UINT32_MAX) return false;
  value = (uint32_t)parsed;
  return true;
}

static bool crcSdTile(const char* path, uint32_t& crc, uint8_t* scratch, size_t scratchBytes) {
  File file = SD_MMC.open(path, FILE_READ);
  if (!file || file.isDirectory() || file.size() != TileUpload::tileBytes) { if (file) file.close(); return false; }
  crc = 0xffffffffU;
  for (size_t offset = 0; offset < TileUpload::tileBytes;) {
    size_t count = min(scratchBytes, TileUpload::tileBytes - offset);
    if (file.read(scratch, count) != count) { file.close(); return false; }
    crc = TileUpload::crcUpdate(crc, scratch, count);
    offset += count;
    delay(0);
  }
  file.close();
  crc ^= 0xffffffffU;
  return true;
}

static bool commitUsbTile() {
  SdLock lock;
  if (!lock || !sdStorage.available) return false;
  char temporary[136], backup[136];
  snprintf(temporary, sizeof(temporary), "%s.part", usbTilePath);
  snprintf(backup, sizeof(backup), "%s.bak", usbTilePath);
  File file = SD_MMC.open(temporary, FILE_WRITE);
  if (!file) return false;
  bool ok = true;
  for (size_t offset = 0; offset < TileUpload::tileBytes; offset += 4096) {
    if (file.write(usbTileBuffer + offset, 4096) != 4096) { ok = false; break; }
    delay(0);
  }
  file.close();
  uint32_t actualCrc = 0;
  if (ok) ok = crcSdTile(temporary, actualCrc, usbTileBuffer, 4096) && actualCrc == usbTileExpectedCrc;
  bool hadOld = SD_MMC.exists(usbTilePath);
  if (ok && hadOld) {
    if (SD_MMC.exists(backup) && !SD_MMC.remove(backup)) ok = false;
    if (ok) ok = SD_MMC.rename(usbTilePath, backup);
  }
  if (ok) {
    ok = SD_MMC.rename(temporary, usbTilePath);
    if (!ok && hadOld) SD_MMC.rename(backup, usbTilePath);
  }
  if (!ok) { SD_MMC.remove(temporary); return false; }
  if (hadOld) SD_MMC.remove(backup);
  sdStorage.writes++;
  mapStorageGeneration.fetch_add(1);
  mapTileCacheResident.store(0);
  mapCanvasCached = false;
  mapRenderPending = true;
  lastMapUiRefreshMs = 0;
  mapTileRootFound = true;
  return true;
}

static bool handleUsbTileCommand(char* command) {
  if (strncmp(command, "tile-", 5) != 0) return false;
  char* save = nullptr;
  char* verb = strtok_r(command, " ", &save);
  if (strcmp(verb, "tile-hello") == 0) {
    Serial.printf("[TILEUSB] 00000000 HELLO protocol=1 bytes=%u chunk=%u zoom=10-14 active=%08lx\n", (unsigned)TileUpload::tileBytes, (unsigned)TileUpload::chunkBytes, (unsigned long)usbTileId);
    return true;
  }
  uint32_t id = 0;
  if (!tileNumber(strtok_r(nullptr, " ", &save), id, 16)) { Serial.println("[TILEUSB] 00000000 ERROR invalid-id"); return true; }
  auto error = [id](const char* reason) { Serial.printf("[TILEUSB] %08lx ERROR %s\n", (unsigned long)id, reason); };
  if (strcmp(verb, "tile-abort") == 0) {
    if (id != 0 && id != usbTileId) { error("wrong-id"); return true; }
    resetUsbTileUpload();
    Serial.printf("[TILEUSB] %08lx ABORTED\n", (unsigned long)id);
    return true;
  }
  if (strcmp(verb, "tile-begin") == 0) {
    uint32_t z, x, y, crc, overwrite;
    bool valid = tileNumber(strtok_r(nullptr, " ", &save), z) && tileNumber(strtok_r(nullptr, " ", &save), x) &&
                 tileNumber(strtok_r(nullptr, " ", &save), y) && tileNumber(strtok_r(nullptr, " ", &save), crc, 16) &&
                 tileNumber(strtok_r(nullptr, " ", &save), overwrite) && !strtok_r(nullptr, " ", &save);
    if (!id || !valid || overwrite > 1 || !TileUpload::validCoordinates(z, x, y)) { error("invalid-header"); return true; }
    if (usbTileUploadActive()) { error("busy"); return true; }
    if (usbSdExportActive()) { error("export-busy"); return true; }
    if (!sdStorage.available) { error("no-sd"); return true; }
    if (mapBenchmarkRunning) { error("benchmark-busy"); return true; }
    SdLock lock;
    if (!lock) { error("sd-busy"); return true; }
    // Use the detected tile root, never a host-supplied filesystem path.
    char zoomPath[96], xPath[112], backup[136];
    snprintf(zoomPath, sizeof(zoomPath), "%s/%lu", mapTileRoot, (unsigned long)z);
    snprintf(xPath, sizeof(xPath), "%s/%lu", zoomPath, (unsigned long)x);
    snprintf(usbTilePath, sizeof(usbTilePath), "%s/%lu.rgb565", xPath, (unsigned long)y);
    snprintf(backup, sizeof(backup), "%s.bak", usbTilePath);
    if (!SD_MMC.exists(usbTilePath) && SD_MMC.exists(backup) && !SD_MMC.rename(backup, usbTilePath)) { error("restore-failed"); return true; }
    // Reuse one PSRAM tile for checksumming existing files and receiving new data.
    usbTileBuffer = (uint8_t*)heap_caps_malloc(TileUpload::tileBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!usbTileBuffer) { error("no-psram"); return true; }
    uint32_t actualCrc = 0;
    if (crcSdTile(usbTilePath, actualCrc, usbTileBuffer, 4096) && actualCrc == crc) {
      resetUsbTileUpload();
      Serial.printf("[TILEUSB] %08lx SKIP %08lx\n", (unsigned long)id, (unsigned long)crc);
      return true;
    }
    if (SD_MMC.exists(usbTilePath) && !overwrite) { resetUsbTileUpload(); error("exists-use-overwrite"); return true; }
    bool dirs = (SD_MMC.exists("/s3-lora") || SD_MMC.mkdir("/s3-lora")) &&
                (SD_MMC.exists(mapTileRoot) || SD_MMC.mkdir(mapTileRoot)) &&
                (SD_MMC.exists(zoomPath) || SD_MMC.mkdir(zoomPath)) && (SD_MMC.exists(xPath) || SD_MMC.mkdir(xPath));
    if (!dirs) { resetUsbTileUpload(); error("mkdir-failed"); return true; }
    usbTileId = id;
    usbTileExpectedCrc = crc;
    usbTileReceived = 0;
    usbTileLastMs = millis();
    Serial.printf("[TILEUSB] %08lx READY\n", (unsigned long)id);
    return true;
  }
  if (!usbTileUploadActive() || id != usbTileId) { error("no-matching-upload"); return true; }
  usbTileLastMs = millis();
  if (strcmp(verb, "tile-data") == 0) {
    uint32_t offset;
    char* offsetText = strtok_r(nullptr, " ", &save);
    char* encoded = strtok_r(nullptr, " ", &save);
    if (!tileNumber(offsetText, offset) || !encoded || strtok_r(nullptr, " ", &save)) { error("invalid-chunk"); return true; }
    // Decode into unused staging space only after checking the maximum decoded length.
    size_t length = strlen(encoded), decoded = 0;
    if (!length || length > 4096 || length % 4 || offset != usbTileReceived || usbTileReceived >= TileUpload::tileBytes) { error("invalid-offset-or-size"); return true; }
    size_t padding = (encoded[length - 1] == '=') + (length > 1 && encoded[length - 2] == '=');
    size_t count = length / 4 * 3 - padding;
    if (!TileUpload::validChunk(usbTileReceived, offset, count) || mbedtls_base64_decode(usbTileBuffer + usbTileReceived, TileUpload::tileBytes - usbTileReceived, &decoded, (const unsigned char*)encoded, length) != 0 || decoded != count) { error("invalid-base64"); return true; }
    usbTileReceived += decoded;
    Serial.printf("[TILEUSB] %08lx ACK %u\n", (unsigned long)id, (unsigned)usbTileReceived);
    return true;
  }
  if (strcmp(verb, "tile-end") == 0 && !strtok_r(nullptr, " ", &save)) {
    if (usbTileReceived != TileUpload::tileBytes) { error("incomplete"); return true; }
    uint32_t crc = TileUpload::crcUpdate(0xffffffffU, usbTileBuffer, usbTileReceived) ^ 0xffffffffU;
    if (crc != usbTileExpectedCrc) { resetUsbTileUpload(); error("crc-mismatch"); return true; }
    bool ok = commitUsbTile();
    resetUsbTileUpload();
    if (ok) Serial.printf("[TILEUSB] %08lx DONE %08lx\n", (unsigned long)id, (unsigned long)crc);
    else { sdStorage.writeErrors++; error("sd-commit-failed"); }
    return true;
  }
  error("unknown-command");
  return true;
}

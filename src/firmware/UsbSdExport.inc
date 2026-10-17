// Read-only access to the installed SD card for copying to a USB card reader.
// Paths are base64 UTF-8; data is base64 with an independent CRC32 per chunk.
static File usbSdFile;
static uint8_t* usbSdBuffer = nullptr;
static uint32_t usbSdId = 0, usbSdLastMs = 0, usbSdSize = 0, usbSdNext = 0, usbSdPrevious = UINT32_MAX;
static uint32_t usbSdCrc = 0xffffffffU;
static constexpr size_t USB_SD_CHUNK = 24576;
static constexpr size_t USB_SD_ENCODED = USB_SD_CHUNK / 3 * 4 + 1;

static bool usbSdExportActive() { return usbSdBuffer != nullptr; }

static void closeUsbSdExport() {
  SdLock lock;
  if (usbSdFile) usbSdFile.close();
  if (usbSdBuffer) heap_caps_free(usbSdBuffer);
  usbSdBuffer = nullptr;
  usbSdId = 0;
}

static void serviceUsbSdTimeout() {
  if (usbSdExportActive() && (uint32_t)(millis() - usbSdLastMs) > 30000) closeUsbSdExport();
}

static bool usbSdDecodePath(const char* encoded, char* path, size_t capacity) {
  if (!encoded || strlen(encoded) > 256) return false;
  size_t length = 0;
  if (mbedtls_base64_decode((unsigned char*)path, capacity - 1, &length, (const unsigned char*)encoded, strlen(encoded)) != 0 || !length || length >= capacity) return false;
  path[length] = 0;
  if (path[0] != '/' || strstr(path, "..") || strstr(path, "//")) return false;
  for (size_t i = 0; i < length; ++i) if ((uint8_t)path[i] < 32 || path[i] == '\\' || path[i] == ':') return false;
  return true;
}

static bool handleUsbSdCommand(char* command) {
  if (strncmp(command, "sd-", 3) != 0) return false;
  char* save = nullptr;
  char* verb = strtok_r(command, " ", &save);
  if (strcmp(verb, "sd-hello") == 0) {
    Serial.printf("[SDUSB] 00000000 HELLO protocol=1 chunk=%u readonly=1\n", (unsigned)USB_SD_CHUNK);
    return true;
  }
  // Parsing helper is defined in UsbTiles.inc; keep export independent of module ordering.
  char* token = strtok_r(nullptr, " ", &save);
  uint32_t id = 0;
  bool validId = token && strlen(token) == 8;
  if (validId) {
    for (const char* p = token; *p; ++p) if (!isxdigit((unsigned char)*p)) validId = false;
    if (validId) id = (uint32_t)strtoul(token, nullptr, 16);
  }
  if (!validId) { Serial.println("[SDUSB] 00000000 ERROR invalid-id"); return true; }
  auto error = [id](const char* reason) { Serial.printf("[SDUSB] %08lx ERROR %s\n", (unsigned long)id, reason); };
  if (strcmp(verb, "sd-close") == 0) {
    if (id != 0 && id != usbSdId) { error("wrong-id"); return true; }
    closeUsbSdExport();
    Serial.printf("[SDUSB] %08lx CLOSED\n", (unsigned long)id);
    return true;
  }
  if (!sdStorage.available || usbTileUploadActive() || mapBenchmarkRunning) { error("sd-unavailable-or-busy"); return true; }
  SdLock lock;
  if (!lock) { error("sd-busy"); return true; }
  usbSdLastMs = millis();
  if (strcmp(verb, "sd-list") == 0 || strcmp(verb, "sd-open") == 0) {
    char path[192];
    if (!usbSdDecodePath(strtok_r(nullptr, " ", &save), path, sizeof(path))) { error("invalid-path"); return true; }
    char* offsetText = strtok_r(nullptr, " ", &save);
    if (!offsetText || strlen(offsetText) > 10 || strtok_r(nullptr, " ", &save)) { error("invalid-offset"); return true; }
    for (const char* p = offsetText; *p; ++p) if (!isdigit((unsigned char)*p)) { error("invalid-offset"); return true; }
    errno = 0;
    unsigned long parsed = strtoul(offsetText, nullptr, 10);
    if (errno || parsed > UINT32_MAX) { error("invalid-offset"); return true; }
    uint32_t offset = (uint32_t)parsed;
    if (strcmp(verb, "sd-list") == 0) {
      File directory = SD_MMC.open(path, FILE_READ);
      if (!directory || !directory.isDirectory()) { if (directory) directory.close(); error("not-directory"); return true; }
      uint32_t cursor = 0, emitted = 0;
      File child = directory.openNextFile();
      while (child && cursor < offset) { child.close(); child = directory.openNextFile(); ++cursor; delay(0); }
      while (child && emitted < 32) {
        char childPath[192], encoded[257];
        int n = snprintf(childPath, sizeof(childPath), "%s%s%s", path, strcmp(path, "/") == 0 ? "" : "/", pathBasename(child.name()));
        if (n < 0 || (size_t)n >= sizeof(childPath)) { child.close(); directory.close(); error("path-too-long"); return true; }
        size_t encodedLength = 0;
        mbedtls_base64_encode((unsigned char*)encoded, sizeof(encoded), &encodedLength, (const unsigned char*)childPath, strlen(childPath));
        encoded[encodedLength] = 0;
        Serial.printf("[SDUSB] %08lx ENTRY %c %lu %s\n", (unsigned long)id, child.isDirectory() ? 'D' : 'F', (unsigned long)child.size(), encoded);
        child.close();
        ++cursor; ++emitted;
        child = directory.openNextFile();
        delay(0);
      }
      bool more = (bool)child;
      if (child) child.close();
      directory.close();
      Serial.printf("[SDUSB] %08lx LIST %lu %u\n", (unsigned long)id, (unsigned long)cursor, more ? 1U : 0U);
      return true;
    }
    if (!id || offset != 0) { error("open-requires-zero-offset"); return true; }
    closeUsbSdExport();
    usbSdFile = SD_MMC.open(path, FILE_READ);
    if (!usbSdFile || usbSdFile.isDirectory() || usbSdFile.size() > UINT32_MAX) { closeUsbSdExport(); error("not-readable-file"); return true; }
    usbSdBuffer = (uint8_t*)heap_caps_malloc(USB_SD_CHUNK + USB_SD_ENCODED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!usbSdBuffer) { closeUsbSdExport(); error("no-psram"); return true; }
    usbSdId = id;
    usbSdSize = (uint32_t)usbSdFile.size();
    usbSdNext = 0;
    usbSdPrevious = UINT32_MAX;
    usbSdCrc = 0xffffffffU;
    Serial.printf("[SDUSB] %08lx OPEN %lu\n", (unsigned long)id, (unsigned long)usbSdSize);
    return true;
  }
  if (strcmp(verb, "sd-read") == 0) {
    char* offsetText = strtok_r(nullptr, " ", &save);
    if (!usbSdExportActive() || id != usbSdId) { error("no-matching-export"); return true; }
    if (!offsetText || !*offsetText || strlen(offsetText) > 10 || strtok_r(nullptr, " ", &save)) { error("invalid-offset"); return true; }
    for (const char* p = offsetText; *p; ++p) if (!isdigit((unsigned char)*p)) { error("invalid-offset"); return true; }
    errno = 0;
    unsigned long parsed = strtoul(offsetText, nullptr, 10);
    if (errno || parsed > usbSdSize) { error("invalid-offset"); return true; }
    uint32_t offset = (uint32_t)parsed;
    if (offset == usbSdSize && offset == usbSdNext) {
      Serial.printf("[SDUSB] %08lx EOF %08lx\n", (unsigned long)id, (unsigned long)(usbSdCrc ^ 0xffffffffU));
      return true;
    }
    if (offset != usbSdNext && offset != usbSdPrevious) { error("nonsequential-offset"); return true; }
    size_t count = min(USB_SD_CHUNK, (size_t)(usbSdSize - offset));
    if (!usbSdFile.seek(offset) || usbSdFile.read(usbSdBuffer, count) != count) { error("short-read"); return true; }
    uint32_t crc = TileUpload::crcUpdate(0xffffffffU, usbSdBuffer, count) ^ 0xffffffffU;
    if (offset == usbSdNext) { usbSdCrc = TileUpload::crcUpdate(usbSdCrc, usbSdBuffer, count); usbSdPrevious = offset; usbSdNext += count; }
    unsigned char* encoded = usbSdBuffer + USB_SD_CHUNK;
    size_t encodedLength = 0;
    if (mbedtls_base64_encode(encoded, USB_SD_ENCODED, &encodedLength, usbSdBuffer, count) != 0) { error("encode-failed"); return true; }
    Serial.printf("[SDUSB] %08lx DATA %lu %08lx ", (unsigned long)id, (unsigned long)offset, (unsigned long)crc);
    Serial.write(encoded, encodedLength);
    Serial.println();
    return true;
  }
  error("unknown-command");
  return true;
}

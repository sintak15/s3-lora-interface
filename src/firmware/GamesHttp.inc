// Authenticated SD ROM management. Uploads are staged, validated, then renamed.
static File gameUploadFile;
static String gameUploadName;
static size_t gameUploadBytes = 0;
static uint8_t gameUploadHeader[512] = {};
static int gameUploadCode = 400;
static String gameUploadMessage = "No ROM received";
static bool gameUploadActive = false, gameUploadSeen = false;
static void gameUploadCleanup() {
  SdLock lock(50);
  if (lock) { if (gameUploadFile) gameUploadFile.close(); sdCardFilesystem().remove(String(GAME_ROM_DIR) + "/.upload.part"); }
  gameUploadActive = false;
}
static void handleGameUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (!server.authenticate(webUiUser, webUiPass)) { gameUploadCode = 401; gameUploadMessage = "Authentication required"; return; }
    if (usingDefaultWebCredentials()) { gameUploadCode = 403; gameUploadMessage = "Finish credential setup first"; return; }
    if (gameUploadSeen) { gameUploadCode = 400; gameUploadMessage = "Upload one ROM at a time"; gameUploadCleanup(); return; }
    gameUploadSeen = true; gameUploadCode = 400; gameUploadMessage = "Upload incomplete"; gameUploadBytes = 0;
    memset(gameUploadHeader, 0, sizeof(gameUploadHeader)); gameUploadName = upload.filename;
    if (!RomFiles::validName(gameUploadName.c_str())) { gameUploadMessage = "Use a .gb filename up to 64 characters; letters, numbers, spaces, dash, underscore and dot"; return; }
    if (!sdStorage.available) { gameUploadCode = 503; gameUploadMessage = String("SD not mounted: ") + sdStorage.status + ". Open SD Storage and retry mounting."; return; }
    if (usbTileUploadActive() || usbSdExportActive()) { gameUploadCode = 409; gameUploadMessage = "SD busy with a USB transfer"; return; }
    SdLock lock(50); if (!lock) { gameUploadCode = 409; gameUploadMessage = "SD busy"; return; }
    sdCardFilesystem().mkdir("/s3-lora"); sdCardFilesystem().mkdir("/s3-lora/games"); sdCardFilesystem().mkdir(GAME_ROM_DIR);
    if (sdCardFilesystem().exists(String(GAME_ROM_DIR) + "/" + gameUploadName)) { gameUploadCode = 409; gameUploadMessage = "ROM already exists; delete it first to replace it"; return; }
    sdCardFilesystem().remove(String(GAME_ROM_DIR) + "/.upload.part");
    gameUploadFile = sdCardFilesystem().open(String(GAME_ROM_DIR) + "/.upload.part", FILE_WRITE);
    if (!gameUploadFile) { gameUploadCode = 507; gameUploadMessage = "Could not create ROM file"; return; }
    gameUploadActive = true;
  } else if (upload.status == UPLOAD_FILE_WRITE && gameUploadActive) {
    if (gameUploadBytes + upload.currentSize > RomFiles::MAX_ROM_BYTES) { gameUploadCode = 413; gameUploadMessage = "ROM exceeds 4 MiB"; gameUploadCleanup(); return; }
    SdLock lock(50); if (!lock) { gameUploadCode = 409; gameUploadMessage = "SD busy; upload cancelled"; gameUploadActive = false; return; }
    size_t take = gameUploadBytes < sizeof(gameUploadHeader) ? min(upload.currentSize, sizeof(gameUploadHeader) - gameUploadBytes) : 0;
    if (take) memcpy(gameUploadHeader + gameUploadBytes, upload.buf, take);
    if (gameUploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) { gameUploadCode = 507; gameUploadMessage = "SD write failed"; gameUploadFile.close(); sdCardFilesystem().remove(String(GAME_ROM_DIR) + "/.upload.part"); gameUploadActive = false; return; }
    gameUploadBytes += upload.currentSize;
    pollLoRa(); pollLocalGps(); serviceRadioApiKeepalive();
  } else if (upload.status == UPLOAD_FILE_END && gameUploadActive) {
    SdLock lock(50); if (!lock) { gameUploadCode = 409; gameUploadMessage = "SD busy; upload cancelled"; gameUploadCleanup(); return; }
    gameUploadFile.flush(); gameUploadFile.close(); gameUploadActive = false;
    if (!RomFiles::validHeader(gameUploadHeader, gameUploadBytes)) { gameUploadMessage = "Invalid cartridge header, size or Color-only ROM"; sdCardFilesystem().remove(String(GAME_ROM_DIR) + "/.upload.part"); return; }
    if (!sdCardFilesystem().rename(String(GAME_ROM_DIR) + "/.upload.part", String(GAME_ROM_DIR) + "/" + gameUploadName)) { gameUploadCode = 500; gameUploadMessage = "Could not finalize ROM"; return; }
    gameUploadCode = 201; gameUploadMessage = "ROM uploaded. Refresh the touchscreen ROM list.";
  } else if (upload.status == UPLOAD_FILE_ABORTED) { gameUploadMessage = "Upload interrupted"; gameUploadCleanup(); gameUploadSeen = false; }
}
static void registerGamesRoutes() {
  server.on("/games/status", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    server.send(200, "application/json", String("{\"running\":") + (gbRunning ? "true" : "false") + ",\"frames\":" + gbFrames + ",\"rom\":\"" + jsonEscape(gbFileName) + "\",\"saveDirty\":" + (gbRamDirty ? "true" : "false") + ",\"keys\":" + unsigned(uint8_t(~gbTouchKeys)) + ",\"touchReads\":" + gbTouchReads + ",\"touchPresses\":" + gbTouchPresses + ",\"fpsTenths\":" + gbFpsTenths + ",\"presented\":" + gbPresented + ",\"frameUs\":" + gbFrameUs + ",\"peakFrameUs\":" + gbPeakFrameUs + "}");
  });
#ifdef FIRMWARE_RUN_TESTS
  server.on("/diagnostics/games", HTTP_POST, []() { if (requireWebAuth()) server.send(200, "application/json", verifyGamesRuntime()); });
  server.on("/diagnostics/games/performance", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    if (gbCore) { server.send(409, "text/plain", "Close the running game before benchmarking"); return; }
    const String name = server.arg("name");
    if (!RomFiles::validName(name.c_str())) { server.send(400, "text/plain", "Choose an installed ROM"); return; }
    auto* savedPage = currentPage; const uint8_t depth = navigationDepth;
    lv_obj_t* stack[8]; memcpy(stack, navigationStack, sizeof(stack));
    if (!startGameBoy(name.c_str())) { server.send(500, "text/plain", "Could not load ROM"); return; }
    gbRunning = false;
    uint32_t timings[2] = {};
    for (unsigned render = 0; render < 2; ++render) {
      const uint32_t start = micros();
      for (unsigned i = 0; i < 120 && !gameBoyCoreError(gbCore); ++i) {
        gameBoyCoreFrameRender(gbCore, 0xff, render);
        pollLoRa(); pollLocalGps(); serviceRadioApiKeepalive(); delay(1);
      }
      timings[render] = micros() - start;
    }
    gbFrames = gbPresented = 0; gbFpsWindowFrames = 0; gbFpsWindowMs = millis();
    gbRunning = true; gbNextFrameUs = micros();
    const uint32_t start = millis();
    while (millis() - start < 8000 && !gameBoyCoreError(gbCore)) {
      // This benchmark deliberately discards its private RAM changes. Never
      // autosave over the player's cartridge RAM while measuring neutral input.
      gbLastSave = millis();
      serviceGames(); serviceScreen(); pollLoRa(); pollLocalGps(); serviceRadioApiKeepalive(); delay(2);
    }
    const uint32_t elapsed = millis() - start;
    String result = String("{\"frames\":") + gbFrames + ",\"presented\":" + gbPresented + ",\"elapsedMs\":" + elapsed +
      ",\"core120Us\":" + timings[0] + ",\"render120Us\":" + timings[1] + ",\"error\":" + gameBoyCoreError(gbCore) + "}";
    gbRamDirty = false; releaseGameBoy(); showPage(savedPage, false);
    navigationDepth = depth; memcpy(navigationStack, stack, sizeof(stack));
    server.send(200, "application/json", result);
  });
#endif
  server.on("/games/roms", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    if (!sdStorage.available) { server.send(503, "text/plain", String("SD not mounted: ") + sdStorage.status + ". Open SD Storage and retry mounting."); return; }
    SdLock lock(50); if (!lock) { server.send(503, "text/plain", "SD busy; retry refreshing the ROM list"); return; }
    String json = "{\"roms\":["; unsigned count = 0;
    File dir = sdCardFilesystem().open(GAME_ROM_DIR);
    if (dir && dir.isDirectory()) {
      for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
        String path = entry.name(), name = path.substring(path.lastIndexOf('/') + 1);
        if (!entry.isDirectory() && RomFiles::validName(name.c_str()) && count < 128) {
          if (count++) json += ',';
          json += "{\"name\":\"" + jsonEscape(name.c_str()) + "\",\"bytes\":" + String(entry.size()) + "}";
        }
        entry.close();
      }
      dir.close();
    }
    json += "],\"maxBytes\":" + String(RomFiles::MAX_ROM_BYTES) + "}"; server.send(200, "application/json", json);
  });
  server.on("/games/roms/upload", HTTP_POST, []() {
    if (!requireWebAuth()) { gameUploadCleanup(); gameUploadSeen = false; return; }
    gameUploadCleanup(); server.send(gameUploadCode, "text/plain", gameUploadMessage); gameUploadSeen = false;
  }, handleGameUpload);
  server.on("/games/roms/delete", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    String name = server.arg("name");
    if (!RomFiles::validName(name.c_str())) { server.send(400, "text/plain", "Invalid ROM name"); return; }
    if (gameUploadActive || (gbRunning && name == gbFileName)) { server.send(409, "text/plain", "ROM is in use"); return; }
    SdLock lock(50); if (!lock || !sdStorage.available) { server.send(503, "text/plain", "SD unavailable"); return; }
    String path = String(GAME_ROM_DIR) + "/" + name;
    if (!sdCardFilesystem().exists(path)) { server.send(404, "text/plain", "ROM not found"); return; }
    server.send(sdCardFilesystem().remove(path) ? 200 : 500, "text/plain", "ROM deletion finished; save files retained");
  });
}

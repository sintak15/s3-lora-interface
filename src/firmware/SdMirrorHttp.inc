static bool validSdMirrorPath(const String& path) {
  if (!path.length() || path.length() > 180 || path[0] != '/' || strlen(path.c_str()) != path.length() || path.indexOf("..") >= 0 || path.indexOf("//") >= 0) return false;
  for (size_t i = 0; i < path.length(); ++i) if ((uint8_t)path[i] < 32 || path[i] == '\\' || path[i] == ':') return false;
  return true;
}
static void registerSdMirrorRoutes() {
  server.on("/sd/gpt/verify", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    if (!GptStorage::mounted) { server.send(409, "text/plain", "No GPT partition mounted"); return; }
    SdLock lock(100); if (!lock) { server.send(503, "text/plain", "SD busy"); return; }
    uint32_t actual = 0; esp_err_t result = GptStorage::checksumMetadata(actual);
    bool unchanged = result == ESP_OK && actual == GptStorage::metadataCrc;
    server.send(unchanged ? 200 : 500, "application/json", String("{\"unchanged\":") + (unchanged ? "true" : "false") + ",\"expectedCrc\":" + GptStorage::metadataCrc + ",\"actualCrc\":" + actual + "}");
  });
  server.on("/sd/list", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    String path = server.hasArg("path") ? server.arg("path") : String("/");
    if (!validSdMirrorPath(path)) { server.send(400, "text/plain", "Invalid SD path"); return; }
    uint32_t cursor = server.hasArg("cursor") ? formU32("cursor") : 0;
    SdLock lock(100); if (!lock || !sdStorage.available) { server.send(503, "text/plain", "SD unavailable"); return; }
    File dir = sdCardFilesystem().open(path);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); server.send(404, "text/plain", "Directory not found"); return; }
    String json = "{\"entries\":["; uint32_t position = 0, count = 0; bool more = false;
    for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
      if (position++ < cursor) { entry.close(); continue; }
      if (count == 64) { more = true; entry.close(); break; }
      String name = entry.name(); name = name.substring(name.lastIndexOf('/') + 1);
      String full = path == "/" ? "/" + name : path + "/" + name;
      if (count++) json += ',';
      json += "{\"path\":\"" + jsonEscape(full.c_str()) + "\",\"directory\":" + String(entry.isDirectory() ? "true" : "false") + ",\"bytes\":" + String(entry.size()) + "}";
      entry.close();
    }
    dir.close(); json += "],\"next\":" + String(cursor + count) + ",\"more\":" + String(more ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });
  server.on("/sd/file", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    String path = server.arg("path");
    if (!validSdMirrorPath(path)) { server.send(400, "text/plain", "Invalid SD path"); return; }
    String name = path.substring(path.lastIndexOf('/') + 1);
    handleSdDownload(path.c_str(), name.c_str(), "application/octet-stream");
  });
}

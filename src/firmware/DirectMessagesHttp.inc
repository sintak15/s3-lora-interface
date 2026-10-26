static uint32_t requestedDirectPeer(const String& value) {
  if (value.length() != 9 || value[0] != '!') return 0;
  for (unsigned i = 1; i < 9; ++i) if (!isxdigit((unsigned char)value[i])) return 0;
  uint32_t result = strtoul(value.c_str() + 1, nullptr, 16);
  return result == BROADCAST_ADDR ? 0 : result;
}
static String conversationListJson() {
  String json = "{\"conversations\":["; unsigned count = 0;
  for (const auto& thread : directConversations) if (thread.peer) {
    char id[12]; snprintf(id, sizeof(id), "!%08lx", (unsigned long)thread.peer);
    if (count++) json += ',';
    json += "{\"peer\":\"" + String(id) + "\",\"name\":\"" + jsonEscape(nodeName(thread.peer)) + "\",\"unread\":" + String(thread.unread) + ",\"order\":" + String(thread.updated) + "}";
  }
  json += "],\"legacy\":" + String(legacyDirectMessages && legacyDirectMessages[0] ? "true" : "false") + "}";
  return json;
}
static void registerDirectMessageRoutes() {
  server.on("/messages/conversations", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    server.send(200, "application/json", conversationListJson());
  });
  server.on("/messages/thread", HTTP_GET, []() {
    if (!requireWebAuth()) return;
    String requested = server.arg("peer");
    if (requested == "legacy") { server.send(200, "application/json", "{\"peer\":\"legacy\",\"text\":\"" + jsonEscape(legacyDirectMessages ? legacyDirectMessages : "") + "\"}"); return; }
    uint32_t peer = requestedDirectPeer(requested);
    if (!peer) { server.send(400, "text/plain", "A valid peer node ID is required"); return; }
    const auto* thread = directConversation(peer);
    server.send(200, "application/json", "{\"peer\":\"" + jsonEscape(requested.c_str()) + "\",\"text\":\"" + jsonEscape(thread ? thread->history : "") + "\"}");
  });
  server.on("/messages/read", HTTP_POST, []() {
    if (!requireWebAuth()) return;
    uint32_t peer = requestedDirectPeer(server.arg("peer"));
    if (!peer) { server.send(400, "text/plain", "A valid peer node ID is required"); return; }
    auto* thread = directConversation(peer); if (thread) thread->unread = 0;
    directInboxDirty = true; updateDirectUnread(); markUiDirty(UI_DIRTY_DASHBOARD);
    server.send(200, "text/plain", "Conversation read");
  });
}

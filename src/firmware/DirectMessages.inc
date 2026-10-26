#include "../ConversationRouting.h"
static const char* DIRECT_THREADS_PATH = "/s3-lora/direct_threads.log";
static const char* DIRECT_LEGACY_PATH = "/s3-lora/direct_legacy.log";
struct DirectConversation {
  uint32_t peer = 0, updated = 0;
  uint16_t unread = 0;
  char* history = nullptr;
  char draft[234] = "";
};
static DirectConversation directConversations[MAX_NODES];
static uint32_t selectedDirectPeer = 0, directConversationOrder = 0;
static char* legacyDirectMessages = nullptr;
static bool directLegacyView = false, directInboxDirty = true;
static char newDirectDraft[234] = "";
static DirectConversation* directConversation(uint32_t peer, bool create = false) {
  if (!peer || peer == BROADCAST_ADDR) return nullptr;
  for (auto& thread : directConversations) if (thread.peer == peer) return &thread;
  if (create) for (auto& thread : directConversations) if (!thread.peer) {
    thread.history = (char*)heap_caps_calloc(CHAT_SIZE, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!thread.history) return nullptr;
    thread.peer = peer; return &thread;
  }
  return nullptr;
}
static uint32_t currentDirectPeer() { return selectedDirectPeer; }
static void updateDirectUnread() {
  unsigned total = 0; for (const auto& thread : directConversations) total += thread.unread;
  unreadDirect = min(total, 999U);
}
static void markDirectConversationRead() {
  auto* thread = directConversation(selectedDirectPeer);
  if (thread && thread->unread) { thread->unread = 0; directInboxDirty = true; updateDirectUnread(); }
}
static const char* currentDirectConversationText() {
  if (directLegacyView) return legacyDirectMessages && legacyDirectMessages[0] ? legacyDirectMessages : "No earlier messages";
  auto* thread = directConversation(selectedDirectPeer);
  return thread && thread->history[0] ? thread->history : "No messages with this device yet";
}
static void storeDirectConversation(uint32_t peer, const char* line, bool local) {
  if (!peer || !line) return;
  char record[880]; snprintf(record, sizeof(record), "%08lx\t%s", (unsigned long)peer, line);
  appendSdLine(DIRECT_THREADS_PATH, record);
  auto* thread = directConversation(peer, true);
  if (thread) {
    appendLine(thread->history, CHAT_SIZE, line); thread->updated = ++directConversationOrder;
    if (!local && !(currentPage == pageDirectChat && selectedDirectPeer == peer) && thread->unread < 999) ++thread->unread;
  }
  directInboxDirty = true; updateDirectUnread(); markUiDirty(UI_DIRTY_CHAT | UI_DIRTY_DASHBOARD);
}
static void initDirectMessages() {
  if (!legacyDirectMessages) legacyDirectMessages = (char*)heap_caps_calloc(CHAT_SIZE, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!legacyDirectMessages) return;
  SdLock lock;
  if (!lock || !sdStorage.available) { strlcpy(legacyDirectMessages, directChatLog, CHAT_SIZE); return; }
  if (!sdCardFilesystem().exists(DIRECT_THREADS_PATH)) {
    // Old outgoing records omit recipients; retain them without inventing peers.
    if (!sdCardFilesystem().exists(DIRECT_LEGACY_PATH) && sdCardFilesystem().exists(SD_DIRECT_CHAT_PATH)) {
      sdCardFilesystem().remove(String(DIRECT_LEGACY_PATH) + ".tmp");
      File source = sdCardFilesystem().open(SD_DIRECT_CHAT_PATH), dest = sdCardFilesystem().open(String(DIRECT_LEGACY_PATH) + ".tmp", FILE_WRITE);
      bool ok = source && dest; uint8_t chunk[512];
      while (ok && source.available()) { size_t n = source.read(chunk, sizeof(chunk)); if (!n || dest.write(chunk, n) != n) ok = false; }
      if (source) source.close(); if (dest) { dest.flush(); dest.close(); }
      if (!ok || !sdCardFilesystem().rename(String(DIRECT_LEGACY_PATH) + ".tmp", DIRECT_LEGACY_PATH)) { strlcpy(legacyDirectMessages, directChatLog, CHAT_SIZE); return; }
    }
    File journal = sdCardFilesystem().open(DIRECT_THREADS_PATH, FILE_WRITE); if (journal) { journal.print("# direct conversations v1\n"); journal.close(); }
  }
  loadSdTextTail(DIRECT_LEGACY_PATH, legacyDirectMessages, CHAT_SIZE);
  File file = sdCardFilesystem().open(DIRECT_THREADS_PATH); if (!file) return;
  // Bound startup RAM and IO; full journal remains on SD for export.
  if (file.size() > 128 * 1024) { file.seek(file.size() - 128 * 1024); file.readStringUntil('\n'); }
  DirectConversation* current = nullptr;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    bool prefix = line.length() >= 9 && line[8] == '\t';
    for (unsigned i = 0; prefix && i < 8; ++i) prefix = isxdigit((unsigned char)line[i]);
    if (prefix) {
      char id[9]; memcpy(id, line.c_str(), 8); id[8] = 0;
      current = directConversation(strtoul(id, nullptr, 16), true); line.remove(0, 9);
      if (current) current->updated = ++directConversationOrder;
    }
    if (current) { line += '\n'; appendLine(current->history, CHAT_SIZE, line.c_str()); }
  }
  file.close(); directInboxDirty = true;
}
static void openDirectConversation(uint32_t peer, bool legacy) {
  if (taDirectInput && !directLegacyView) {
    auto* old = directConversation(selectedDirectPeer);
    strlcpy(old ? old->draft : newDirectDraft, lv_textarea_get_text(taDirectInput), 234);
  }
  selectedDirectPeer = peer; directLegacyView = legacy;
  if (peer) directConversation(peer, true);
  char address[12] = ""; if (peer) snprintf(address, sizeof(address), "!%08lX", (unsigned long)peer);
  if (taDirectTo) lv_textarea_set_text(taDirectTo, address);
  if (taDirectInput) { auto* next = directConversation(peer); lv_textarea_set_text(taDirectInput, legacy ? "" : next ? next->draft : newDirectDraft); }
  if (directThreadTitle) lv_label_set_text(directThreadTitle, legacy ? "Earlier messages" : peer ? nodeName(peer) : "New conversation");
  chatScrollToNewestPending = true; markUiDirty(UI_DIRTY_CHAT);
  showPage(pageDirectChat); markDirectConversationRead();
}
static void refreshDirectInbox() {
  if (currentPage != pageDirectInbox || !directInboxList || !directInboxDirty || uiReadInteractionActive()) return;
  auto anchor = UiScroll::capture(directInboxList); lv_obj_clean(directInboxList);
  unsigned order[MAX_NODES], count = 0;
  for (unsigned i = 0; i < MAX_NODES; ++i) if (directConversations[i].peer) order[count++] = i;
  for (unsigned i = 0; i < count; ++i) for (unsigned j = i + 1; j < count; ++j) if (directConversations[order[j]].updated > directConversations[order[i]].updated) { unsigned t = order[i]; order[i] = order[j]; order[j] = t; }
  for (unsigned k = 0; k < count; ++k) {
    auto& thread = directConversations[order[k]]; char title[112];
    snprintf(title, sizeof(title), "%s  (%u new)\n!%08lX", nodeName(thread.peer), thread.unread, (unsigned long)thread.peer);
    auto* button = lv_list_add_btn(directInboxList, nullptr, title); styleDarkObject(button, COLOR_PANEL);
    lv_obj_set_style_text_font(button, UI_FONT_SMALL, 0); lv_obj_set_height(button, 55); lv_obj_set_user_data(button, (void*)(uintptr_t)thread.peer);
    lv_obj_add_event_cb(button, [](lv_event_t* e) { openDirectConversation((uintptr_t)lv_event_get_user_data(e)); }, LV_EVENT_CLICKED, (void*)(uintptr_t)thread.peer);
  }
  if (legacyDirectMessages && legacyDirectMessages[0]) {
    auto* button = lv_list_add_btn(directInboxList, nullptr, "Earlier messages\nRecipients not recorded"); styleDarkObject(button, COLOR_PANEL); lv_obj_set_height(button, 55); lv_obj_set_style_text_font(button, UI_FONT_SMALL, 0);
    lv_obj_add_event_cb(button, [](lv_event_t*) { openDirectConversation(0, true); }, LV_EVENT_CLICKED, nullptr);
  }
  UiScroll::restore(directInboxList, anchor); directInboxDirty = false;
}

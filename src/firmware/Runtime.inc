// Internal implementation module; included once by the main sketch.
static void markUiDirty(uint32_t flags) {
  uiDirtyFlags |= flags;
}

static bool isUiDirty(uint32_t flags) {
  return (uiDirtyFlags & flags) != 0;
}

static void clearUiDirty(uint32_t flags) {
  uiDirtyFlags &= ~flags;
}

static void* allocatePsramBuffer(size_t bytes, const char* name) {
  void* ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  const char* source = "PSRAM";
  if (!ptr) {
    ptr = heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    source = "internal";
  }
  if (ptr) {
    memset(ptr, 0, bytes);
    Serial.printf("[boot] %s buffer %u bytes in %s\n", name, (unsigned)bytes, source);
  } else {
    Serial.printf("[boot] %s buffer allocation failed (%u bytes)\n", name, (unsigned)bytes);
  }
  return ptr;
}

static void allocateRuntimeBuffers() {
  if (!eventLog) eventLog = (char*)allocatePsramBuffer(LOG_SIZE, "eventLog");
  if (!packetLog) packetLog = (char*)allocatePsramBuffer(PACKET_LOG_SIZE, "packetLog");
  if (!publicChatLog) publicChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "publicChat");
  if (!familyChatLog) familyChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "familyChat");
  if (!directChatLog) directChatLog = (char*)allocatePsramBuffer(CHAT_SIZE, "directChat");
  if (!mapCanvasBuf) {
    mapCanvasBuf = (lv_color_t*)allocatePsramBuffer(MAP_CANVAS_BYTES, "mapCanvas");
  }
  if (!mapWorkerBuf) {
    mapWorkerBuf = (lv_color_t*)allocatePsramBuffer(MAP_CANVAS_BYTES, "mapWorker");
  }
}

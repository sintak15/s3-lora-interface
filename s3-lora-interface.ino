#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "constants.h"
#include "src/meshtastic/mesh.pb.h"
#include "src/meshtastic/telemetry.pb.h"
#include "src/meshtastic/portnums.pb.h"
#include "src/UIConfig.h"

HardwareSerial SerialLoRa(2);
WebServer server(80);
TFT_eSPI tft;

struct NodeRecord {
  uint32_t num = 0;
  char name[40] = "";
  float snr = 0.0f;
  uint32_t lastHeardMs = 0;
};

struct ChannelRecord {
  int8_t index = -1;
  char name[12] = "";
  bool enabled = false;
};

struct DeviceStats {
  uint32_t myNodeNum = 0;
  uint32_t uptimeSeconds = 0;
  uint32_t batteryLevel = 0;
  float voltage = 0.0f;
  float channelUtilization = 0.0f;
  float airUtilTx = 0.0f;
  uint32_t packetsRx = 0;
  uint32_t packetsTx = 0;
  uint16_t onlineNodes = 0;
  uint16_t totalNodes = 0;
};

struct GpsStats {
  bool valid = false;
  bool hasSats = false;
  bool hasFixQuality = false;
  bool hasFixType = false;
  bool hasTimestamp = false;
  bool hasPdop = false;
  bool hasHdop = false;
  bool hasVdop = false;
  bool hasAccuracy = false;
  bool hasGroundSpeed = false;
  bool hasGroundTrack = false;
  bool hasNextUpdate = false;
  bool hasPrecision = false;
  bool hasAltitudeHae = false;
  bool hasGeoidalSeparation = false;
  uint32_t from = 0;
  double latitude = 0.0;
  double longitude = 0.0;
  int32_t altitude = 0;
  int32_t altitudeHae = 0;
  int32_t geoidalSeparation = 0;
  uint32_t sats = 0;
  uint32_t fixQuality = 0;
  uint32_t fixType = 0;
  uint32_t timestamp = 0;
  uint32_t pdop = 0;
  uint32_t hdop = 0;
  uint32_t vdop = 0;
  uint32_t accuracyMm = 0;
  uint32_t groundSpeed = 0;
  uint32_t groundTrack = 0;
  uint32_t sensorId = 0;
  uint32_t nextUpdate = 0;
  uint32_t seqNumber = 0;
  uint32_t precisionBits = 0;
  uint32_t lastUpdateMs = 0;
  char sourceKind[24] = "none";
};

struct LocalBatteryStats {
  uint32_t rawMv = 0;
  uint32_t batteryMv = 0;
  int32_t deltaMvPerMin = 0;
  int percent = 0;
  bool usbLikely = false;
  bool chargingLikely = false;
  uint32_t lastSampleMs = 0;
  uint32_t lastTrendMs = 0;
  uint32_t lastTrendMv = 0;
  char powerState[24] = "unknown";
};

static constexpr size_t LOG_SIZE = 8192;
static constexpr size_t CHAT_SIZE = 4096;
static constexpr size_t MAX_NODES = 64;
static constexpr size_t MAX_CHANNELS = 8;
static constexpr size_t FRAME_MAX = 512;
static constexpr int NAV_BAR_H = 40;
static constexpr int8_t PUBLIC_CHANNEL_INDEX = 0;

static char eventLog[LOG_SIZE];
static char publicChatLog[CHAT_SIZE];
static char privateChatLog[CHAT_SIZE];
static NodeRecord nodes[MAX_NODES];
static ChannelRecord channels[MAX_CHANNELS];
static size_t nodeCount = 0;
static int8_t privateChannelIndex = -1;
static DeviceStats stats;
static GpsStats gpsStats;
static LocalBatteryStats localBattery;
static uint32_t framesDecoded = 0;
static uint32_t decodeErrors = 0;
static uint32_t bytesFromRadio = 0;
static uint32_t bytesToRadio = 0;
static uint32_t lastByteMs = 0;
static uint32_t magic1Count = 0;
static uint32_t magic2Count = 0;
static uint32_t streamFrames = 0;
static uint32_t invalidFrameLengths = 0;
static uint32_t textPackets = 0;
static uint32_t telemetryPackets = 0;
static uint32_t positionPackets = 0;
static uint32_t remotePositionPackets = 0;
static uint32_t nodeInfoPackets = 0;
static uint32_t encryptedPackets = 0;
static uint32_t configFrames = 0;
static uint32_t otherFrames = 0;
static uint32_t lastPortNum = 0;
static char serialPeek[96] = "";
static uint32_t lastTelemetryMs = 0;
static uint32_t lastConfigRequestMs = 0;
static uint8_t configRequestCount = 0;

static lv_disp_draw_buf_t drawBuf;
static lv_color_t lvBuf1[SCREEN_W * 24];
static lv_color_t lvBuf2[SCREEN_W * 24];
static lv_obj_t* pageLauncher = nullptr;
static lv_obj_t* pageLora = nullptr;
static lv_obj_t* pagePublicChat = nullptr;
static lv_obj_t* pagePrivateChat = nullptr;
static lv_obj_t* pageGps = nullptr;
static lv_obj_t* pageSystem = nullptr;
static lv_obj_t* pageBattery = nullptr;
static lv_obj_t* currentPage = nullptr;
static lv_obj_t* previousPage = nullptr;
static lv_obj_t* navBar = nullptr;
static lv_obj_t* lblStatus = nullptr;
static lv_obj_t* lblStats = nullptr;
static lv_obj_t* lblSystemSummary = nullptr;
static lv_obj_t* lblBatteryStats = nullptr;
static lv_obj_t* lblGpsStats = nullptr;
static lv_obj_t* taPublicChat = nullptr;
static lv_obj_t* taPrivateChat = nullptr;
static lv_obj_t* taScreenLog = nullptr;
static lv_obj_t* taScreenNodes = nullptr;
static lv_obj_t* taPublicInput = nullptr;
static lv_obj_t* taPrivateInput = nullptr;
static lv_obj_t* activeChatInput = nullptr;
static lv_obj_t* keyboard = nullptr;
static uint32_t lastUiRefreshMs = 0;
static uint32_t lastSerialDiagMs = 0;

static bool sendTextMessage(const char* text, int8_t channelIndex = PUBLIC_CHANNEL_INDEX);

static int batteryPercentFromMv(uint32_t mv) {
  if (mv >= 4200) return 100;
  if (mv <= 3300) return 0;
  return constrain((int)((mv - 3300) * 100 / 900), 0, 100);
}

static void sampleLocalBattery() {
  if (millis() - localBattery.lastSampleMs < 1000) return;
  localBattery.lastSampleMs = millis();

  uint32_t adcMv = analogReadMilliVolts(BATT_ADC_PIN);
  uint32_t packMv = (uint32_t)(adcMv * BATT_ADC_MULTIPLIER + 0.5f);
  if (localBattery.batteryMv == 0) {
    localBattery.batteryMv = packMv;
  } else {
    localBattery.batteryMv = (localBattery.batteryMv * 7 + packMv) / 8;
  }
  localBattery.rawMv = adcMv;
  localBattery.percent = batteryPercentFromMv(localBattery.batteryMv);

  if (localBattery.lastTrendMs == 0) {
    localBattery.lastTrendMs = millis();
    localBattery.lastTrendMv = localBattery.batteryMv;
  } else if (millis() - localBattery.lastTrendMs >= 30000) {
    int32_t delta = (int32_t)localBattery.batteryMv - (int32_t)localBattery.lastTrendMv;
    uint32_t elapsedMs = millis() - localBattery.lastTrendMs;
    localBattery.deltaMvPerMin = (int32_t)((delta * 60000L) / (int32_t)elapsedMs);
    localBattery.lastTrendMs = millis();
    localBattery.lastTrendMv = localBattery.batteryMv;
  }

  localBattery.usbLikely = localBattery.batteryMv >= 4300;
  localBattery.chargingLikely = localBattery.batteryMv >= 4100 && localBattery.deltaMvPerMin > 4;

  if (localBattery.usbLikely) {
    strlcpy(localBattery.powerState, "USB/external", sizeof(localBattery.powerState));
  } else if (localBattery.chargingLikely) {
    strlcpy(localBattery.powerState, "charging", sizeof(localBattery.powerState));
  } else if (localBattery.deltaMvPerMin < -4) {
    strlcpy(localBattery.powerState, "battery only", sizeof(localBattery.powerState));
  } else {
    strlcpy(localBattery.powerState, "battery/steady", sizeof(localBattery.powerState));
  }
}

static void appendLine(char* buffer, size_t bufferSize, const char* line) {
  if (!buffer || !line || bufferSize == 0) return;
  size_t used = strlen(buffer);
  size_t incoming = strlen(line);
  if (used + incoming + 1 >= bufferSize) {
    const char* marker = "--- log trimmed ---\n";
    size_t markerLen = strlen(marker);
    size_t keep = bufferSize / 2;
    memmove(buffer, buffer + used - keep, keep);
    memmove(buffer + markerLen, buffer, keep);
    memcpy(buffer, marker, markerLen);
    buffer[markerLen + keep] = '\0';
    used = strlen(buffer);
  }
  strncat(buffer, line, bufferSize - used - 1);
}

static void lvFlush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* colors) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)colors, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

static bool readTouch(uint16_t& x, uint16_t& y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)5) != 5) return false;

  uint8_t touches = Wire.read();
  uint8_t xh = Wire.read();
  uint8_t xl = Wire.read();
  uint8_t yh = Wire.read();
  uint8_t yl = Wire.read();
  if (touches == 0 || touches > 2) return false;

  x = constrain(((uint16_t)(xh & 0x0F) << 8) | xl, 0, SCREEN_W - 1);
  y = constrain(((uint16_t)(yh & 0x0F) << 8) | yl, 0, SCREEN_H - 1);
  return true;
}

static void lvTouchRead(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  uint16_t x = 0;
  uint16_t y = 0;
  if (readTouch(x, y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static lv_obj_t* makePanel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x101816), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x24483E), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 6, 0);
  lv_obj_set_style_pad_all(panel, 8, 0);
  return panel;
}

static lv_obj_t* makePage(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H - NAV_BAR_H);
  lv_obj_align(page, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(page, lv_color_hex(0x050807), 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 8, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  return page;
}

static void showPage(lv_obj_t* target, bool remember = true) {
  if (!target) return;
  if (remember && currentPage && currentPage != target) previousPage = currentPage;
  lv_obj_t* pages[] = {pageLauncher, pageLora, pagePublicChat, pagePrivateChat, pageGps, pageSystem, pageBattery};
  for (lv_obj_t* page : pages) {
    if (!page) continue;
    if (page == target) lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  if (keyboard) lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  currentPage = target;
}

static lv_obj_t* makeActionButton(lv_obj_t* parent, const char* text, int y, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, SCREEN_W - 24, 52);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x101816), 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x2F705F), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0xF4FFF9), 0);
  lv_obj_center(label);
  return btn;
}

static lv_obj_t* makePageTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* title = lv_label_create(parent);
  lv_label_set_text(title, text);
  lv_obj_set_style_text_color(title, lv_color_hex(0x68FFC0), 0);
  lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
  return title;
}

static lv_obj_t* makeNavButton(lv_obj_t* parent, const char* text, lv_align_t align, int x, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 92, 30);
  lv_obj_align(btn, align, x, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x111A18), 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x315B50), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_radius(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0xF4FFF9), 0);
  lv_obj_center(label);
  return btn;
}

static void buildNavBar(lv_obj_t* screen) {
  navBar = lv_obj_create(screen);
  lv_obj_set_size(navBar, SCREEN_W, NAV_BAR_H);
  lv_obj_align(navBar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(navBar, lv_color_hex(0x080D0B), 0);
  lv_obj_set_style_bg_opa(navBar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(navBar, lv_color_hex(0x24483E), 0);
  lv_obj_set_style_border_side(navBar, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(navBar, 1, 0);
  lv_obj_set_style_pad_all(navBar, 4, 0);
  lv_obj_clear_flag(navBar, LV_OBJ_FLAG_SCROLLABLE);

  makeNavButton(navBar, "Back", LV_ALIGN_LEFT_MID, 4, [](lv_event_t*) {
    if (previousPage && currentPage != pageLauncher) {
      lv_obj_t* target = previousPage;
      previousPage = pageLauncher;
      showPage(target, false);
    } else {
      showPage(pageLauncher, false);
    }
  });
  makeNavButton(navBar, "Launcher", LV_ALIGN_RIGHT_MID, -4, [](lv_event_t*) {
    previousPage = currentPage;
    showPage(pageLauncher, false);
  });
}

static lv_obj_t* makeReadonlyText(lv_obj_t* parent, int y, int h) {
  lv_obj_t* ta = lv_textarea_create(parent);
  lv_obj_set_size(ta, SCREEN_W - 16, h);
  lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, y);
  lv_textarea_set_cursor_click_pos(ta, false);
  lv_obj_clear_flag(ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_set_style_text_font(ta, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_bg_color(ta, lv_color_hex(0x07100D), 0);
  lv_obj_set_style_text_color(ta, lv_color_hex(0xE8FFF5), 0);
  lv_obj_set_style_border_color(ta, lv_color_hex(0x2F705F), 0);
  return ta;
}

static int8_t activeChatChannel() {
  if (activeChatInput == taPrivateInput) {
    return privateChannelIndex >= 0 ? privateChannelIndex : 1;
  }
  return PUBLIC_CHANNEL_INDEX;
}

static void sendFromInput(lv_obj_t* input, int8_t channelIndex) {
  if (!input) return;
  const char* msg = lv_textarea_get_text(input);
  if (msg && msg[0] && sendTextMessage(msg, channelIndex)) {
    lv_textarea_set_text(input, "");
  }
}

static void sendActiveFromScreen() {
  sendFromInput(activeChatInput, activeChatChannel());
}

static void inputEvent(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (!keyboard) return;
  if (code == LV_EVENT_FOCUSED) {
    activeChatInput = (lv_obj_t*)lv_event_get_target(e);
    lv_keyboard_set_textarea(keyboard, activeChatInput);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(keyboard, nullptr);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_READY) {
    sendActiveFromScreen();
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  } else if (code == LV_EVENT_CANCEL) {
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    if (activeChatInput) lv_obj_clear_state(activeChatInput, LV_STATE_FOCUSED);
  }
}

static void buildScreenUi() {
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x050807), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(0xF4FFF9), 0);

  lblStatus = lv_label_create(screen);
  lv_label_set_text(lblStatus, "booting...");
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x8AB7A6), 0);
  lv_obj_set_style_text_font(lblStatus, LV_FONT_DEFAULT, 0);
  lv_obj_align(lblStatus, LV_ALIGN_TOP_RIGHT, -8, 8);

  pageLauncher = makePage(screen);
  pageLora = makePage(screen);
  pagePublicChat = makePage(screen);
  pagePrivateChat = makePage(screen);
  pageGps = makePage(screen);
  pageSystem = makePage(screen);
  pageBattery = makePage(screen);

  currentPage = pageLauncher;
  lv_obj_clear_flag(pageLauncher, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* launcherTitle = lv_label_create(pageLauncher);
  lv_label_set_text(launcherTitle, "Heltec Interface");
  lv_obj_set_style_text_color(launcherTitle, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_style_text_font(launcherTitle, LV_FONT_DEFAULT, 0);
  lv_obj_align(launcherTitle, LV_ALIGN_TOP_LEFT, 4, 18);

  lv_obj_t* launcherSub = lv_label_create(pageLauncher);
  lv_label_set_text(launcherSub, "Select a module");
  lv_obj_set_style_text_color(launcherSub, lv_color_hex(0x8AB7A6), 0);
  lv_obj_align(launcherSub, LV_ALIGN_TOP_LEFT, 4, 42);

  makeActionButton(pageLauncher, "LoRa", 86, [](lv_event_t*) { showPage(pageLora); });
  makeActionButton(pageLauncher, "GPS", 148, [](lv_event_t*) { showPage(pageGps); });
  makeActionButton(pageLauncher, "System", 210, [](lv_event_t*) { showPage(pageSystem); });

  makePageTitle(pageLora, "LoRa");
  makeActionButton(pageLora, "Public Chat", 76, [](lv_event_t*) { showPage(pagePublicChat); });
  makeActionButton(pageLora, "Private Chat", 138, [](lv_event_t*) { showPage(pagePrivateChat); });

  lv_obj_t* loraPanel = makePanel(pageLora);
  lv_obj_set_size(loraPanel, SCREEN_W - 16, 74);
  lv_obj_align(loraPanel, LV_ALIGN_TOP_MID, 0, 202);
  lblStats = lv_label_create(loraPanel);
  lv_label_set_text(lblStats, "Waiting for radio...");
  lv_obj_set_style_text_font(lblStats, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_text_color(lblStats, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_width(lblStats, lv_pct(100));

  makePageTitle(pagePublicChat, "Public Chat");
  lv_obj_t* publicStats = lv_label_create(pagePublicChat);
  lv_label_set_text(publicStats, "Channel: primary");
  lv_obj_set_style_text_color(publicStats, lv_color_hex(0x8AB7A6), 0);
  lv_obj_align(publicStats, LV_ALIGN_TOP_LEFT, 4, 34);

  taPublicChat = makeReadonlyText(pagePublicChat, 54, 146);
  lv_textarea_set_text(taPublicChat, "No public chat yet");
  taPublicInput = lv_textarea_create(pagePublicChat);
  lv_obj_set_size(taPublicInput, SCREEN_W - 76, 38);
  lv_obj_align(taPublicInput, LV_ALIGN_TOP_LEFT, 8, 210);
  lv_obj_set_style_bg_color(taPublicInput, lv_color_hex(0x07100D), 0);
  lv_obj_set_style_text_color(taPublicInput, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_style_border_color(taPublicInput, lv_color_hex(0x2F705F), 0);
  lv_textarea_set_one_line(taPublicInput, true);
  lv_textarea_set_max_length(taPublicInput, 233);
  lv_textarea_set_placeholder_text(taPublicInput, "Public message");
  lv_obj_add_event_cb(taPublicInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* publicSendBtn = lv_btn_create(pagePublicChat);
  lv_obj_set_size(publicSendBtn, 56, 38);
  lv_obj_align(publicSendBtn, LV_ALIGN_TOP_RIGHT, -8, 210);
  lv_obj_set_style_bg_color(publicSendBtn, lv_color_hex(0x00C985), 0);
  lv_obj_set_style_shadow_width(publicSendBtn, 0, 0);
  lv_obj_add_event_cb(publicSendBtn, [](lv_event_t*) { sendFromInput(taPublicInput, PUBLIC_CHANNEL_INDEX); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* publicSendLbl = lv_label_create(publicSendBtn);
  lv_label_set_text(publicSendLbl, "Send");
  lv_obj_set_style_text_color(publicSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(publicSendLbl);

  makePageTitle(pagePrivateChat, "Private Chat");
  lv_obj_t* privateStats = lv_label_create(pagePrivateChat);
  lv_label_set_text(privateStats, "Channel: priv");
  lv_obj_set_style_text_color(privateStats, lv_color_hex(0x8AB7A6), 0);
  lv_obj_align(privateStats, LV_ALIGN_TOP_LEFT, 4, 34);

  taPrivateChat = makeReadonlyText(pagePrivateChat, 54, 146);
  lv_textarea_set_text(taPrivateChat, "No private chat yet");
  taPrivateInput = lv_textarea_create(pagePrivateChat);
  lv_obj_set_size(taPrivateInput, SCREEN_W - 76, 38);
  lv_obj_align(taPrivateInput, LV_ALIGN_TOP_LEFT, 8, 210);
  lv_obj_set_style_bg_color(taPrivateInput, lv_color_hex(0x07100D), 0);
  lv_obj_set_style_text_color(taPrivateInput, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_style_border_color(taPrivateInput, lv_color_hex(0x2F705F), 0);
  lv_textarea_set_one_line(taPrivateInput, true);
  lv_textarea_set_max_length(taPrivateInput, 233);
  lv_textarea_set_placeholder_text(taPrivateInput, "Private message");
  lv_obj_add_event_cb(taPrivateInput, inputEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* privateSendBtn = lv_btn_create(pagePrivateChat);
  lv_obj_set_size(privateSendBtn, 56, 38);
  lv_obj_align(privateSendBtn, LV_ALIGN_TOP_RIGHT, -8, 210);
  lv_obj_set_style_bg_color(privateSendBtn, lv_color_hex(0x00C985), 0);
  lv_obj_set_style_shadow_width(privateSendBtn, 0, 0);
  lv_obj_add_event_cb(privateSendBtn, [](lv_event_t*) { sendFromInput(taPrivateInput, privateChannelIndex >= 0 ? privateChannelIndex : 1); }, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* privateSendLbl = lv_label_create(privateSendBtn);
  lv_label_set_text(privateSendLbl, "Send");
  lv_obj_set_style_text_color(privateSendLbl, lv_color_hex(0x001B12), 0);
  lv_obj_center(privateSendLbl);

  lv_obj_t* privHint = lv_label_create(pagePrivateChat);
  lv_label_set_text(privHint, "Meshtastic channel name: priv");
  lv_obj_set_style_text_color(privHint, lv_color_hex(0x8AB7A6), 0);
  lv_obj_align(privHint, LV_ALIGN_TOP_LEFT, 4, 252);

  makePageTitle(pageGps, "GPS");
  lv_obj_t* gpsPanel = makePanel(pageGps);
  lv_obj_set_size(gpsPanel, SCREEN_W - 16, 214);
  lv_obj_align(gpsPanel, LV_ALIGN_TOP_MID, 0, 48);
  lblGpsStats = lv_label_create(gpsPanel);
  lv_label_set_text(lblGpsStats, "Waiting for GPS from Heltec...");
  lv_obj_set_style_text_color(lblGpsStats, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_width(lblGpsStats, lv_pct(100));

  makePageTitle(pageSystem, "System");
  lv_obj_t* systemPanel = makePanel(pageSystem);
  lv_obj_set_size(systemPanel, SCREEN_W - 16, 190);
  lv_obj_align(systemPanel, LV_ALIGN_TOP_MID, 0, 48);
  lblSystemSummary = lv_label_create(systemPanel);
  lv_label_set_text(lblSystemSummary, "System ready");
  lv_obj_set_style_text_color(lblSystemSummary, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_width(lblSystemSummary, lv_pct(100));
  makeActionButton(pageSystem, "Battery", 238, [](lv_event_t*) { showPage(pageBattery); });

  makePageTitle(pageBattery, "Battery");
  lv_obj_t* batteryPanel = makePanel(pageBattery);
  lv_obj_set_size(batteryPanel, SCREEN_W - 16, 222);
  lv_obj_align(batteryPanel, LV_ALIGN_TOP_MID, 0, 48);
  lblBatteryStats = lv_label_create(batteryPanel);
  lv_label_set_text(lblBatteryStats, "Reading S3 battery...");
  lv_obj_set_style_text_color(lblBatteryStats, lv_color_hex(0xF4FFF9), 0);
  lv_obj_set_width(lblBatteryStats, lv_pct(100));

  buildNavBar(screen);

  keyboard = lv_keyboard_create(screen);
  lv_obj_set_size(keyboard, SCREEN_W, 132);
  lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x101816), 0);
  lv_obj_set_style_text_color(keyboard, lv_color_hex(0xF4FFF9), 0);
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void initScreen() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  lv_init();
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, SCREEN_W * 24);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_W;
  dispDrv.ver_res = SCREEN_H;
  dispDrv.flush_cb = lvFlush;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvTouchRead;
  lv_indev_drv_register(&indevDrv);

  buildScreenUi();
}

static void refreshScreenUi() {
  if (!lblStatus || millis() - lastUiRefreshMs < 500) return;
  lastUiRefreshMs = millis();

  char status[48];
  snprintf(status, sizeof(status), "%lu frames", (unsigned long)framesDecoded);
  lv_label_set_text(lblStatus, status);

  char statsText[256];
  snprintf(statsText, sizeof(statsText),
           "Node: %s\nID: !%08lX\nRX/TX: %lu/%lu  Nodes: %u/%u\nPriv: %s",
           nodeName(stats.myNodeNum),
           (unsigned long)stats.myNodeNum,
           (unsigned long)stats.packetsRx,
           (unsigned long)stats.packetsTx,
           stats.onlineNodes,
           stats.totalNodes,
           privateChannelIndex >= 0 ? "found" : "not found");
  lv_label_set_text(lblStats, statsText);

  if (lblSystemSummary) {
    char systemText[420];
    char rxAge[32];
    if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lus ago", (unsigned long)((millis() - lastByteMs) / 1000));
    else strlcpy(rxAge, "never", sizeof(rxAge));
    snprintf(systemText, sizeof(systemText),
             "Interface uptime: %lus\nFree heap: %lu KB\nFree PSRAM: %lu KB\nAP: %s\n\n"
             "Serial link\n"
             "RX bytes: %lu  TX bytes: %lu\n"
             "Last byte: %s\n"
             "Magic 94/C3: %lu/%lu\n"
             "Frames: %lu  Decode errors: %lu\n"
             "Bad lengths: %lu\n"
             "Text/Tel/GPS/Node: %lu/%lu/%lu/%lu\n"
             "Remote GPS ignored: %lu\n"
             "Config/Other/Encrypted: %lu/%lu/%lu\n"
             "Last port: %lu\n"
             "ASCII seen: %.48s",
             (unsigned long)(millis() / 1000),
             (unsigned long)(ESP.getFreeHeap() / 1024),
             (unsigned long)(ESP.getFreePsram() / 1024),
             WiFi.softAPIP().toString().c_str(),
             (unsigned long)bytesFromRadio,
             (unsigned long)bytesToRadio,
             rxAge,
             (unsigned long)magic1Count,
             (unsigned long)magic2Count,
             (unsigned long)streamFrames,
             (unsigned long)decodeErrors,
             (unsigned long)invalidFrameLengths,
             (unsigned long)textPackets,
             (unsigned long)telemetryPackets,
             (unsigned long)positionPackets,
             (unsigned long)nodeInfoPackets,
             (unsigned long)remotePositionPackets,
             (unsigned long)configFrames,
             (unsigned long)otherFrames,
             (unsigned long)encryptedPackets,
             (unsigned long)lastPortNum,
             serialPeek);
    lv_label_set_text(lblSystemSummary, systemText);
  }

  if (lblBatteryStats) {
    char batteryText[420];
    snprintf(batteryText, sizeof(batteryText),
             "S3 battery\n"
             "Level: %d%%\n"
             "Voltage: %.2f V\n"
             "ADC pin: GPIO%d\n"
             "ADC reading: %lu mV\n"
             "Power: %s\n"
             "Trend: %ld mV/min\n\n"
             "S3 interface power\n"
             "Uptime: %lu s\n"
             "Heap free/min: %lu/%lu KB\n"
             "PSRAM free: %lu KB\n\n"
             "Heltec radio\n"
             "Packets RX/TX: %lu/%lu\n"
             "Channel use: %.2f%%\n"
             "Air TX use: %.2f%%",
             localBattery.percent,
             localBattery.batteryMv / 1000.0f,
             BATT_ADC_PIN,
             (unsigned long)localBattery.rawMv,
             localBattery.powerState,
             (long)localBattery.deltaMvPerMin,
             (unsigned long)(millis() / 1000),
             (unsigned long)(ESP.getFreeHeap() / 1024),
             (unsigned long)(ESP.getMinFreeHeap() / 1024),
             (unsigned long)(ESP.getFreePsram() / 1024),
             (unsigned long)stats.packetsRx,
             (unsigned long)stats.packetsTx,
             stats.channelUtilization,
             stats.airUtilTx);
    lv_label_set_text(lblBatteryStats, batteryText);
  }

  if (lblGpsStats) {
    char gpsText[720];
    if (gpsStats.valid) {
      uint32_t age = (millis() - gpsStats.lastUpdateMs) / 1000;
      char satsText[24];
      char fixQualityText[32];
      char fixTypeText[24];
      char timeText[32];
      char dopText[64];
      char accuracyText[32];
      char speedText[32];
      char trackText[32];
      char nextUpdateText[32];
      char precisionText[32];
      char altExtraText[72];
      if (gpsStats.hasSats) snprintf(satsText, sizeof(satsText), "%lu", (unsigned long)gpsStats.sats);
      else strlcpy(satsText, "not sent", sizeof(satsText));
      if (gpsStats.hasFixQuality) snprintf(fixQualityText, sizeof(fixQualityText), "%lu", (unsigned long)gpsStats.fixQuality);
      else strlcpy(fixQualityText, "not sent", sizeof(fixQualityText));
      if (gpsStats.hasFixType) snprintf(fixTypeText, sizeof(fixTypeText), "%luD", (unsigned long)gpsStats.fixType);
      else strlcpy(fixTypeText, "not sent", sizeof(fixTypeText));
      if (gpsStats.hasTimestamp) snprintf(timeText, sizeof(timeText), "%lu", (unsigned long)gpsStats.timestamp);
      else strlcpy(timeText, "not sent", sizeof(timeText));
      if (gpsStats.hasPdop || gpsStats.hasHdop || gpsStats.hasVdop) {
        snprintf(dopText, sizeof(dopText), "P %.2f  H %.2f  V %.2f",
                 gpsStats.pdop / 100.0f, gpsStats.hdop / 100.0f, gpsStats.vdop / 100.0f);
      } else {
        strlcpy(dopText, "not sent", sizeof(dopText));
      }
      if (gpsStats.hasAccuracy) snprintf(accuracyText, sizeof(accuracyText), "%.1f m", gpsStats.accuracyMm / 1000.0f);
      else strlcpy(accuracyText, "not sent", sizeof(accuracyText));
      if (gpsStats.hasGroundSpeed) snprintf(speedText, sizeof(speedText), "%.1f m/s", gpsStats.groundSpeed / 100.0f);
      else strlcpy(speedText, "not sent", sizeof(speedText));
      if (gpsStats.hasGroundTrack) snprintf(trackText, sizeof(trackText), "%.1f deg", gpsStats.groundTrack / 100.0f);
      else strlcpy(trackText, "not sent", sizeof(trackText));
      if (gpsStats.hasNextUpdate) snprintf(nextUpdateText, sizeof(nextUpdateText), "%lu s", (unsigned long)gpsStats.nextUpdate);
      else strlcpy(nextUpdateText, "not sent", sizeof(nextUpdateText));
      if (gpsStats.hasPrecision) snprintf(precisionText, sizeof(precisionText), "%lu bits", (unsigned long)gpsStats.precisionBits);
      else strlcpy(precisionText, "not sent", sizeof(precisionText));
      char haeText[18];
      char geoidText[18];
      if (gpsStats.hasAltitudeHae) snprintf(haeText, sizeof(haeText), "%ld m", (long)gpsStats.altitudeHae);
      else strlcpy(haeText, "not sent", sizeof(haeText));
      if (gpsStats.hasGeoidalSeparation) snprintf(geoidText, sizeof(geoidText), "%ld m", (long)gpsStats.geoidalSeparation);
      else strlcpy(geoidText, "not sent", sizeof(geoidText));
      snprintf(altExtraText, sizeof(altExtraText), "HAE: %s  Geoid: %s",
               haeText,
               geoidText);
      snprintf(gpsText, sizeof(gpsText),
               "Node: %s\n"
               "Record: %s\n"
               "Latitude:  %.6f\n"
               "Longitude: %.6f\n"
               "Altitude:  %ld m\n"
               "%s\n"
               "Satellites: %s\n"
               "Fix quality: %s\n"
               "Fix type: %s\n"
               "DOP: %s\n"
               "Accuracy: %s\n"
               "Speed: %s\n"
               "Track: %s\n"
               "GPS time: %s\n"
               "Next update: %s\n"
               "Precision: %s\n"
               "Seq/Sensor: %lu/%lu\n"
               "Updated: %lu s ago",
               nodeName(gpsStats.from),
               gpsStats.sourceKind,
               gpsStats.latitude,
               gpsStats.longitude,
               (long)gpsStats.altitude,
               altExtraText,
               satsText,
               fixQualityText,
               fixTypeText,
               dopText,
               accuracyText,
               speedText,
               trackText,
               timeText,
               nextUpdateText,
               precisionText,
               (unsigned long)gpsStats.seqNumber,
               (unsigned long)gpsStats.sensorId,
               (unsigned long)age);
    } else {
      snprintf(gpsText, sizeof(gpsText),
               "Waiting for GPS from Heltec...\n\n"
               "Heltec GPS pins:\n"
               "RX 41, TX 42\n\n"
               "Meshtastic must have GPS enabled.");
    }
    lv_label_set_text(lblGpsStats, gpsText);
  }

  if (taPublicChat) lv_textarea_set_text(taPublicChat, publicChatLog[0] ? publicChatLog : "No public chat yet");
  if (taPrivateChat) lv_textarea_set_text(taPrivateChat, privateChatLog[0] ? privateChatLog : "No private chat yet");
  if (taScreenLog) lv_textarea_set_text(taScreenLog, eventLog[0] ? eventLog : "Waiting for radio data");

  if (taScreenNodes) {
    String text;
    for (size_t i = 0; i < nodeCount; i++) {
      text += "!";
      text += String(nodes[i].num, HEX);
      text += "  ";
      text += nodes[i].name;
      text += "\nSNR ";
      text += String(nodes[i].snr, 1);
      text += "  ";
      text += String((millis() - nodes[i].lastHeardMs) / 1000);
      text += "s ago\n\n";
    }
    lv_textarea_set_text(taScreenNodes, text.length() ? text.c_str() : "No nodes heard yet");
  }
}

static void serviceScreen() {
  sampleLocalBattery();
  refreshScreenUi();
  lv_timer_handler();
}

static void printSerialDiagnostics() {
  if (millis() - lastSerialDiagMs < 1000) return;
  lastSerialDiagMs = millis();

  uint32_t age = lastByteMs ? (millis() - lastByteMs) / 1000 : 0xFFFFFFFF;
  Serial.printf(
    "[LINK] rx=%lu tx=%lu last=%s frames=%lu decoded=%lu err=%lu magic=%lu/%lu badlen=%lu ascii=\"%.60s\"\n",
    (unsigned long)bytesFromRadio,
    (unsigned long)bytesToRadio,
    lastByteMs ? String(age).c_str() : "never",
    (unsigned long)streamFrames,
    (unsigned long)framesDecoded,
    (unsigned long)decodeErrors,
    (unsigned long)magic1Count,
    (unsigned long)magic2Count,
    (unsigned long)invalidFrameLengths,
    serialPeek
  );
  Serial.printf(
    "[PKT] text=%lu telemetry=%lu gps=%lu node=%lu remoteGps=%lu config=%lu other=%lu encrypted=%lu lastPort=%lu\n",
    (unsigned long)textPackets,
    (unsigned long)telemetryPackets,
    (unsigned long)positionPackets,
    (unsigned long)nodeInfoPackets,
    (unsigned long)remotePositionPackets,
    (unsigned long)configFrames,
    (unsigned long)otherFrames,
    (unsigned long)encryptedPackets,
    (unsigned long)lastPortNum
  );
}

static NodeRecord* findOrCreateNode(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num) return &nodes[i];
  }
  if (nodeCount >= MAX_NODES) return nullptr;
  nodes[nodeCount].num = num;
  snprintf(nodes[nodeCount].name, sizeof(nodes[nodeCount].name), "!%08lX", (unsigned long)num);
  nodes[nodeCount].lastHeardMs = millis();
  return &nodes[nodeCount++];
}

static const char* nodeName(uint32_t num) {
  for (size_t i = 0; i < nodeCount; i++) {
    if (nodes[i].num == num && nodes[i].name[0]) return nodes[i].name;
  }
  static char fallback[16];
  snprintf(fallback, sizeof(fallback), "!%08lX", (unsigned long)num);
  return fallback;
}

static const char* channelName(uint8_t index) {
  for (size_t i = 0; i < MAX_CHANNELS; i++) {
    if (channels[i].enabled && channels[i].index == (int8_t)index && channels[i].name[0]) return channels[i].name;
  }
  return index == PUBLIC_CHANNEL_INDEX ? "primary" : "unknown";
}

static void updateChannelRecord(const meshtastic_Channel& channel) {
  if (channel.index < 0 || channel.index >= (int8_t)MAX_CHANNELS) return;
  ChannelRecord& record = channels[channel.index];
  record.index = channel.index;
  record.enabled = channel.role != meshtastic_Channel_Role_DISABLED;
  if (channel.has_settings && channel.settings.name[0]) {
    strlcpy(record.name, channel.settings.name, sizeof(record.name));
  } else if (channel.index == PUBLIC_CHANNEL_INDEX) {
    strlcpy(record.name, "primary", sizeof(record.name));
  } else {
    record.name[0] = '\0';
  }
  if (record.enabled && strcmp(record.name, "priv") == 0) {
    privateChannelIndex = record.index;
  }

  char line[96];
  snprintf(line, sizeof(line), "[radio] channel %d: %s\n", record.index, record.name[0] ? record.name : "(unnamed)");
  appendLine(eventLog, LOG_SIZE, line);
}

static bool isPrivateChannel(uint8_t index) {
  if (privateChannelIndex >= 0) return index == privateChannelIndex;
  return index == 1;
}

static void writeStreamFrame(const uint8_t* payload, size_t len) {
  SerialLoRa.write(0x94);
  SerialLoRa.write(0xC3);
  SerialLoRa.write((len >> 8) & 0xFF);
  SerialLoRa.write(len & 0xFF);
  SerialLoRa.write(payload, len);
  bytesToRadio += len + 4;
}

static bool sendConfigRequest() {
  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_want_config_id_tag;
  toRadio.want_config_id = 1;

  uint8_t out[64];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  writeStreamFrame(out, stream.bytes_written);
  lastConfigRequestMs = millis();
  if (configRequestCount < 255) configRequestCount++;
  appendLine(eventLog, LOG_SIZE, "[local] requested Heltec config\n");
  return true;
}

static void serviceConfigRequests() {
  bool needsInitialData = stats.myNodeNum == 0 || nodeInfoPackets == 0 || positionPackets == 0;
  bool needsChannelData = privateChannelIndex < 0 && configFrames == 0;
  if (!(needsInitialData || needsChannelData)) return;
  if (configRequestCount >= 12) return;
  if (millis() - lastConfigRequestMs < 30000) return;
  sendConfigRequest();
}

static bool sendTextMessage(const char* text, int8_t channelIndex) {
  if (!text || !text[0]) return false;

  meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_zero;
  toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;

  meshtastic_MeshPacket& packet = toRadio.packet;
  packet.to = 0xFFFFFFFF;
  packet.channel = channelIndex < 0 ? PUBLIC_CHANNEL_INDEX : channelIndex;
  packet.want_ack = false;
  packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

  meshtastic_Data& data = packet.decoded;
  data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
  data.payload.size = min(strlen(text), sizeof(data.payload.bytes));
  memcpy(data.payload.bytes, text, data.payload.size);

  uint8_t out[512];
  pb_ostream_t stream = pb_ostream_from_buffer(out, sizeof(out));
  if (!pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio)) return false;
  writeStreamFrame(out, stream.bytes_written);

  char line[300];
  snprintf(line, sizeof(line), "[me] %.*s\n", (int)data.payload.size, text);
  appendLine(isPrivateChannel(packet.channel) ? privateChatLog : publicChatLog, CHAT_SIZE, line);
  snprintf(line, sizeof(line), "[local] text sent on %s\n", channelName(packet.channel));
  appendLine(eventLog, LOG_SIZE, line);
  return true;
}

static void updateTelemetry(uint32_t from, const meshtastic_Data& data) {
  meshtastic_Telemetry telemetry = meshtastic_Telemetry_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
  if (!pb_decode(&stream, meshtastic_Telemetry_fields, &telemetry)) return;

  char line[160];
  if (telemetry.which_variant == meshtastic_Telemetry_device_metrics_tag) {
    lastTelemetryMs = millis();
    stats.batteryLevel = telemetry.variant.device_metrics.battery_level;
    stats.voltage = telemetry.variant.device_metrics.voltage;
    stats.channelUtilization = telemetry.variant.device_metrics.channel_utilization;
    stats.airUtilTx = telemetry.variant.device_metrics.air_util_tx;
    stats.uptimeSeconds = telemetry.variant.device_metrics.uptime_seconds;
    snprintf(line, sizeof(line), "[%s] battery %lu%% %.2fV\n",
             nodeName(from), (unsigned long)stats.batteryLevel, stats.voltage);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (telemetry.which_variant == meshtastic_Telemetry_local_stats_tag) {
    lastTelemetryMs = millis();
    stats.packetsRx = telemetry.variant.local_stats.num_packets_rx;
    stats.packetsTx = telemetry.variant.local_stats.num_packets_tx;
    stats.onlineNodes = telemetry.variant.local_stats.num_online_nodes;
    stats.totalNodes = telemetry.variant.local_stats.num_total_nodes;
    appendLine(eventLog, LOG_SIZE, "[radio] local stats updated\n");
  }
}

static void updatePositionFromStruct(uint32_t from, const meshtastic_Position& position, const char* sourceKind, bool diagnosticsIncluded) {
  if (stats.myNodeNum != 0 && from != stats.myNodeNum) {
    remotePositionPackets++;
    char line[160];
    snprintf(line, sizeof(line), "[%s] remote GPS ignored for local GPS screen\n", nodeName(from));
    appendLine(eventLog, LOG_SIZE, line);
    return;
  }

  gpsStats.valid = position.has_latitude_i && position.has_longitude_i;
  gpsStats.from = from;
  gpsStats.latitude = position.latitude_i / 10000000.0;
  gpsStats.longitude = position.longitude_i / 10000000.0;
  gpsStats.altitude = position.has_altitude ? position.altitude : 0;
  if (position.sats_in_view > 0) {
    gpsStats.sats = position.sats_in_view;
    gpsStats.hasSats = true;
  } else if (diagnosticsIncluded) {
    gpsStats.hasSats = false;
  }
  if (position.fix_quality > 0) {
    gpsStats.fixQuality = position.fix_quality;
    gpsStats.hasFixQuality = true;
  } else if (diagnosticsIncluded) {
    gpsStats.hasFixQuality = false;
  }
  if (position.fix_type > 0) {
    gpsStats.fixType = position.fix_type;
    gpsStats.hasFixType = true;
  } else if (diagnosticsIncluded) {
    gpsStats.hasFixType = false;
  }
  if (position.timestamp > 0) {
    gpsStats.timestamp = position.timestamp;
    gpsStats.hasTimestamp = true;
  } else if (position.time > 0) {
    gpsStats.timestamp = position.time;
    gpsStats.hasTimestamp = true;
  }
  if (position.PDOP > 0) { gpsStats.pdop = position.PDOP; gpsStats.hasPdop = true; }
  if (position.HDOP > 0) { gpsStats.hdop = position.HDOP; gpsStats.hasHdop = true; }
  if (position.VDOP > 0) { gpsStats.vdop = position.VDOP; gpsStats.hasVdop = true; }
  if (position.gps_accuracy > 0) { gpsStats.accuracyMm = position.gps_accuracy; gpsStats.hasAccuracy = true; }
  if (position.has_ground_speed) { gpsStats.groundSpeed = position.ground_speed; gpsStats.hasGroundSpeed = true; }
  if (position.has_ground_track) { gpsStats.groundTrack = position.ground_track; gpsStats.hasGroundTrack = true; }
  if (position.has_altitude_hae) { gpsStats.altitudeHae = position.altitude_hae; gpsStats.hasAltitudeHae = true; }
  if (position.has_altitude_geoidal_separation) {
    gpsStats.geoidalSeparation = position.altitude_geoidal_separation;
    gpsStats.hasGeoidalSeparation = true;
  }
  if (position.next_update > 0) { gpsStats.nextUpdate = position.next_update; gpsStats.hasNextUpdate = true; }
  if (position.precision_bits > 0) { gpsStats.precisionBits = position.precision_bits; gpsStats.hasPrecision = true; }
  gpsStats.sensorId = position.sensor_id;
  gpsStats.seqNumber = position.seq_number;
  strlcpy(gpsStats.sourceKind, sourceKind, sizeof(gpsStats.sourceKind));
  gpsStats.lastUpdateMs = millis();

  char line[160];
  snprintf(line, sizeof(line), "[%s] GPS %.6f, %.6f\n",
           nodeName(from), gpsStats.latitude, gpsStats.longitude);
  appendLine(eventLog, LOG_SIZE, line);
}

static void updatePosition(uint32_t from, const meshtastic_Data& data) {
  meshtastic_Position position = meshtastic_Position_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
  if (!pb_decode(&stream, meshtastic_Position_fields, &position)) return;
  updatePositionFromStruct(from, position, "position packet", true);
}

static void handleDecodedPacket(const meshtastic_MeshPacket& packet) {
  NodeRecord* node = findOrCreateNode(packet.from);
  if (node) {
    node->lastHeardMs = millis();
    node->snr = packet.rx_snr;
  }

  if (packet.which_payload_variant == meshtastic_MeshPacket_encrypted_tag) {
    encryptedPackets++;
    char line[96];
    snprintf(line, sizeof(line), "[%s] encrypted packet\n", nodeName(packet.from));
    appendLine(eventLog, LOG_SIZE, line);
    return;
  }

  if (packet.which_payload_variant != meshtastic_MeshPacket_decoded_tag) return;

  const meshtastic_Data& data = packet.decoded;
  char line[320];

  if (data.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP) {
    textPackets++;
    lastPortNum = data.portnum;
    snprintf(line, sizeof(line), "[%s] %.*s\n", nodeName(packet.from),
             (int)data.payload.size, data.payload.bytes);
    appendLine(isPrivateChannel(packet.channel) ? privateChatLog : publicChatLog, CHAT_SIZE, line);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (data.portnum == meshtastic_PortNum_TELEMETRY_APP) {
    telemetryPackets++;
    lastPortNum = data.portnum;
    updateTelemetry(packet.from, data);
  } else if (data.portnum == meshtastic_PortNum_POSITION_APP) {
    positionPackets++;
    lastPortNum = data.portnum;
    updatePosition(packet.from, data);
  } else if (data.portnum == meshtastic_PortNum_NODEINFO_APP) {
    nodeInfoPackets++;
    lastPortNum = data.portnum;
    meshtastic_User user = meshtastic_User_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data.payload.bytes, data.payload.size);
    if (pb_decode(&stream, meshtastic_User_fields, &user)) {
      NodeRecord* known = findOrCreateNode(packet.from);
      if (known) {
        strncpy(known->name, user.long_name, sizeof(known->name) - 1);
        known->name[sizeof(known->name) - 1] = '\0';
      }
      snprintf(line, sizeof(line), "[radio] node %08lX is %.39s\n",
               (unsigned long)packet.from, user.long_name);
      appendLine(eventLog, LOG_SIZE, line);
    }
  } else {
    lastPortNum = data.portnum;
    otherFrames++;
    snprintf(line, sizeof(line), "[%s] port %d payload %u bytes\n",
             nodeName(packet.from), data.portnum, data.payload.size);
    appendLine(eventLog, LOG_SIZE, line);
  }
}

static void decodeFromRadio(const uint8_t* payload, size_t len) {
  meshtastic_FromRadio fromRadio = meshtastic_FromRadio_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(payload, len);
  if (!pb_decode(&stream, meshtastic_FromRadio_fields, &fromRadio)) {
    decodeErrors++;
    appendLine(eventLog, LOG_SIZE, "[radio] protobuf decode failed\n");
    return;
  }

  framesDecoded++;
  if (fromRadio.which_payload_variant == meshtastic_FromRadio_packet_tag) {
    handleDecodedPacket(fromRadio.packet);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_my_info_tag) {
    stats.myNodeNum = fromRadio.my_info.my_node_num;
    char line[96];
    snprintf(line, sizeof(line), "[radio] connected as !%08lX\n", (unsigned long)stats.myNodeNum);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_node_info_tag) {
    nodeInfoPackets++;
    NodeRecord* node = findOrCreateNode(fromRadio.node_info.num);
    if (node) {
      strncpy(node->name, fromRadio.node_info.user.long_name, sizeof(node->name) - 1);
      node->name[sizeof(node->name) - 1] = '\0';
      node->snr = fromRadio.node_info.snr;
      node->lastHeardMs = millis();
    }
    if (fromRadio.node_info.has_position) {
      positionPackets++;
      updatePositionFromStruct(fromRadio.node_info.num, fromRadio.node_info.position, "node info", false);
    }
    if (fromRadio.node_info.has_device_metrics) {
      lastTelemetryMs = millis();
      stats.batteryLevel = fromRadio.node_info.device_metrics.battery_level;
      stats.voltage = fromRadio.node_info.device_metrics.voltage;
      stats.channelUtilization = fromRadio.node_info.device_metrics.channel_utilization;
      stats.airUtilTx = fromRadio.node_info.device_metrics.air_util_tx;
      stats.uptimeSeconds = fromRadio.node_info.device_metrics.uptime_seconds;
    }
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_log_record_tag) {
    char line[420];
    snprintf(line, sizeof(line), "[node log] %.380s\n", fromRadio.log_record.message);
    appendLine(eventLog, LOG_SIZE, line);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_channel_tag) {
    configFrames++;
    updateChannelRecord(fromRadio.channel);
  } else if (fromRadio.which_payload_variant == meshtastic_FromRadio_config_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_moduleConfig_tag ||
             fromRadio.which_payload_variant == meshtastic_FromRadio_config_complete_id_tag) {
    configFrames++;
  } else {
    otherFrames++;
  }
}

static void pollLoRa() {
  enum State { MAGIC_1, MAGIC_2, LEN_1, LEN_2, PAYLOAD };
  static State state = MAGIC_1;
  static uint8_t frame[FRAME_MAX];
  static uint16_t frameLen = 0;
  static uint16_t framePos = 0;

  while (SerialLoRa.available()) {
    uint8_t c = SerialLoRa.read();
    bytesFromRadio++;
    lastByteMs = millis();
    if (c == 0x94) magic1Count++;

    if ((c >= 32 && c <= 126) || c == '\n' || c == '\r') {
      size_t len = strlen(serialPeek);
      if (len >= sizeof(serialPeek) - 2) {
        memmove(serialPeek, serialPeek + 32, sizeof(serialPeek) - 33);
        serialPeek[sizeof(serialPeek) - 33] = '\0';
        len = strlen(serialPeek);
      }
      serialPeek[len++] = (c == '\r') ? ' ' : (char)c;
      serialPeek[len] = '\0';
    }

    switch (state) {
      case MAGIC_1:
        state = (c == 0x94) ? MAGIC_2 : MAGIC_1;
        break;
      case MAGIC_2:
        if (c == 0xC3) {
          magic2Count++;
          state = LEN_1;
        } else {
          state = MAGIC_1;
        }
        break;
      case LEN_1:
        frameLen = ((uint16_t)c) << 8;
        state = LEN_2;
        break;
      case LEN_2:
        frameLen |= c;
        framePos = 0;
        if (frameLen > 0 && frameLen <= FRAME_MAX) {
          streamFrames++;
          state = PAYLOAD;
        } else {
          invalidFrameLengths++;
          state = MAGIC_1;
        }
        break;
      case PAYLOAD:
        frame[framePos++] = c;
        if (framePos >= frameLen) {
          decodeFromRadio(frame, frameLen);
          state = MAGIC_1;
        }
        break;
    }
  }
}

static String jsonEscape(const char* text) {
  String out;
  while (*text) {
    char c = *text++;
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if ((uint8_t)c >= 0x20) {
      out += c;
    }
  }
  return out;
}

static void handleStatus() {
  sampleLocalBattery();
  char rxAge[32];
  if (lastByteMs) snprintf(rxAge, sizeof(rxAge), "%lus ago", (unsigned long)((millis() - lastByteMs) / 1000));
  else strlcpy(rxAge, "never", sizeof(rxAge));
  String json = "{";
  json += "\"title\":\"" + String(UI::Labels::AppTitle) + "\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"frames\":" + String(framesDecoded) + ",";
  json += "\"errors\":" + String(decodeErrors) + ",";
  json += "\"bytes\":" + String(bytesFromRadio) + ",";
  json += "\"txBytes\":" + String(bytesToRadio) + ",";
  json += "\"lastByte\":\"" + jsonEscape(rxAge) + "\",";
  json += "\"magic1\":" + String(magic1Count) + ",";
  json += "\"magic2\":" + String(magic2Count) + ",";
  json += "\"streamFrames\":" + String(streamFrames) + ",";
  json += "\"badLengths\":" + String(invalidFrameLengths) + ",";
  json += "\"textPackets\":" + String(textPackets) + ",";
  json += "\"telemetryPackets\":" + String(telemetryPackets) + ",";
  json += "\"positionPackets\":" + String(positionPackets) + ",";
  json += "\"remotePositionPackets\":" + String(remotePositionPackets) + ",";
  json += "\"nodeInfoPackets\":" + String(nodeInfoPackets) + ",";
  json += "\"configFrames\":" + String(configFrames) + ",";
  json += "\"otherFrames\":" + String(otherFrames) + ",";
  json += "\"encryptedPackets\":" + String(encryptedPackets) + ",";
  json += "\"lastPort\":" + String(lastPortNum) + ",";
  json += "\"serialPeek\":\"" + jsonEscape(serialPeek) + "\",";
  json += "\"myNode\":\"!" + String(stats.myNodeNum, HEX) + "\",";
  json += "\"myNodeName\":\"" + jsonEscape(nodeName(stats.myNodeNum)) + "\",";
  json += "\"battery\":" + String(localBattery.percent) + ",";
  json += "\"voltage\":" + String(localBattery.batteryMv / 1000.0f, 2) + ",";
  json += "\"batterySource\":\"S3\",";
  json += "\"powerState\":\"" + jsonEscape(localBattery.powerState) + "\",";
  json += "\"batteryTrend\":" + String(localBattery.deltaMvPerMin) + ",";
  json += "\"rx\":" + String(stats.packetsRx) + ",";
  json += "\"tx\":" + String(stats.packetsTx) + ",";
  json += "\"online\":" + String(stats.onlineNodes) + ",";
  json += "\"total\":" + String(stats.totalNodes) + ",";
  json += "\"chat\":\"" + jsonEscape(publicChatLog) + "\",";
  json += "\"publicChat\":\"" + jsonEscape(publicChatLog) + "\",";
  json += "\"privateChat\":\"" + jsonEscape(privateChatLog) + "\",";
  json += "\"privateChannel\":" + String(privateChannelIndex) + ",";
  json += "\"log\":\"" + jsonEscape(eventLog) + "\",";
  json += "\"nodes\":[";
  for (size_t i = 0; i < nodeCount; i++) {
    if (i) json += ",";
    json += "{\"num\":\"!" + String(nodes[i].num, HEX) + "\",";
    json += "\"name\":\"" + jsonEscape(nodes[i].name) + "\",";
    json += "\"snr\":" + String(nodes[i].snr, 1) + ",";
    json += "\"age\":" + String((millis() - nodes[i].lastHeardMs) / 1000) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handleSend() {
  if (!server.hasArg("msg")) {
    server.send(400, "text/plain", "missing msg");
    return;
  }
  bool ok = sendTextMessage(server.arg("msg").c_str(), PUBLIC_CHANNEL_INDEX);
  server.send(ok ? 200 : 500, "text/plain", ok ? "sent" : "send failed");
}

static void handleRoot() {
  server.send(200, "text/html", R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Heltec LoRa Interface</title>
<style>
body{margin:0;background:#0b0f0e;color:#e8fff5;font-family:system-ui,Segoe UI,sans-serif}
header{padding:16px 18px;background:#101816;border-bottom:1px solid #1f3d35}
h1{font-size:20px;margin:0}main{display:grid;gap:12px;padding:12px;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
section{background:#111a18;border:1px solid #24483e;border-radius:8px;padding:12px}
h2{font-size:15px;margin:0 0 10px;color:#68ffc0}.stats{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.stat{background:#0b1210;padding:8px;border-radius:6px}.label{color:#8ab7a6;font-size:12px}.value{font-size:18px}
pre{white-space:pre-wrap;overflow:auto;max-height:360px;margin:0;font:13px ui-monospace,Consolas,monospace}
input{box-sizing:border-box;width:100%;padding:12px;background:#07100d;border:1px solid #2f705f;color:#fff;border-radius:6px}
button{margin-top:8px;padding:11px 14px;border:0;border-radius:6px;background:#00c985;color:#001b12;font-weight:700}
table{width:100%;border-collapse:collapse;font-size:13px}td,th{border-bottom:1px solid #203b35;padding:6px;text-align:left}
</style></head><body><header><h1>Heltec LoRa Interface</h1><div id="ip"></div></header><main>
<section><h2>Radio</h2><div class="stats" id="stats"></div><button onclick="fetch('/config',{method:'POST'})">Request Config</button></section>
<section><h2>Send Message</h2><input id="msg" maxlength="233" placeholder="Message to mesh"><button onclick="send()">Send</button></section>
<section><h2>Chat</h2><pre id="chat"></pre></section>
<section><h2>Nodes</h2><table><thead><tr><th>Node</th><th>Name</th><th>SNR</th><th>Age</th></tr></thead><tbody id="nodes"></tbody></table></section>
<section><h2>Serial Link</h2><pre id="serial"></pre></section>
<section><h2>Event Log</h2><pre id="log"></pre></section>
</main><script>
async function refresh(){const s=await (await fetch('/status')).json();ip.textContent='AP '+s.ip;
stats.innerHTML=[['Node',s.myNode],['S3 Battery',s.battery+'% '+s.voltage+'V'],['Power',s.powerState],['Frames',s.frames],['Errors',s.errors],['RX/TX',s.rx+'/'+s.tx],['Nodes',s.online+'/'+s.total]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
chat.textContent=s.chat||'No chat yet';log.textContent=s.log||'Waiting for radio data';
serial.textContent=`RX bytes: ${s.bytes}\nTX bytes: ${s.txBytes}\nLast byte: ${s.lastByte}\nMagic 94/C3: ${s.magic1}/${s.magic2}\nStream frames: ${s.streamFrames}\nBad lengths: ${s.badLengths}\nText/Tel/GPS/Node: ${s.textPackets}/${s.telemetryPackets}/${s.positionPackets}/${s.nodeInfoPackets}\nRemote GPS ignored: ${s.remotePositionPackets}\nConfig/Other/Encrypted: ${s.configFrames}/${s.otherFrames}/${s.encryptedPackets}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}`;
nodes.innerHTML=s.nodes.map(n=>`<tr><td>${n.num}</td><td>${n.name}</td><td>${n.snr}</td><td>${n.age}s</td></tr>`).join('');
}
async function send(){const m=msg.value.trim();if(!m)return;await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(m)});msg.value='';refresh();}
setInterval(refresh,1000);refresh();
</script></body></html>
)HTML");
}

void setup() {
  Serial.begin(115200);
  SerialLoRa.setRxBufferSize(4096);
  SerialLoRa.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  initScreen();

  appendLine(eventLog, LOG_SIZE, "[boot] Heltec LoRa interface starting\n");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(INTERFACE_AP_SSID, INTERFACE_AP_PASS);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/config", HTTP_POST, []() {
    bool ok = sendConfigRequest();
    server.send(ok ? 200 : 500, "text/plain", ok ? "requested" : "request failed");
  });
  server.begin();

  char line[128];
  snprintf(line, sizeof(line), "[boot] AP %s at %s\n", INTERFACE_AP_SSID, WiFi.softAPIP().toString().c_str());
  appendLine(eventLog, LOG_SIZE, line);
  sendConfigRequest();
}

void loop() {
  pollLoRa();
  serviceConfigRequests();
  server.handleClient();
  serviceScreen();
  printSerialDiagnostics();
  delay(2);
}

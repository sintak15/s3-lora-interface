#include <Arduino.h>
#include <limits.h>
#include <errno.h>
#include <lwip/sockets.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <mbedtls/base64.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <ESP_I2S.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <lvgl.h>
#include <TinyGPSPlus.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "constants.h"
#include "src/meshtastic/admin.pb.h"
#include "src/meshtastic/mesh.pb.h"
#include "src/meshtastic/telemetry.pb.h"
#include "src/meshtastic/portnums.pb.h"
#include "src/UIConfig.h"
#include "src/WebUi.h"
#include "src/Reliability.h"

#ifdef FIRMWARE_RUN_TESTS
#include "src/tests/ReliabilityTests.h"
#endif

#include "src/firmware/State.h"
#include "src/firmware/Declarations.h"

#include "src/firmware/Runtime.inc"
#include "src/firmware/Storage.inc"
#include "src/firmware/Gps.inc"
#include "src/firmware/Battery.inc"
#include "src/firmware/Network.inc"
#include "src/firmware/DisplaySettings.inc"
#include "src/firmware/Audio.inc"
#include "src/firmware/Ui.inc"
#include "src/firmware/Map.inc"
#include "src/firmware/UiRefresh.inc"
#include "src/firmware/Radio.inc"
#include "src/firmware/Http.inc"

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("[boot] serial ready");
  initSdMutex();
  initSdAppendWorker();
  allocateRuntimeBuffers();
  initMapRenderWorker();
  nextMeshPacketId = esp_random();
  if (nextMeshPacketId == 0) nextMeshPacketId = 1;
  SerialLoRa.setRxBufferSize(4096);
  SerialLoRa.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("[boot] prefs");
  prefs.begin("s3-lora", false);
  wifiApMode = prefs.getBool("wifiApMode", true);
  readableTextMode = prefs.getBool("readableText", false);
  nodeListViewMode = prefs.getBool("nodeCompact", true) ? NODE_LIST_VIEW_COMPACT : NODE_LIST_VIEW_DETAILED;
  toneVolumePercent = constrain((int)prefs.getUChar("toneVol", 75), 0, 100);
  toneBoostLevel = clampToneBoost(prefs.getUChar("toneBoost", 2));
  toneCodecProfile = clampToneCodecProfile(prefs.getUChar("toneProfile", TONE_CODEC_PROFILE_DRIVER));
  toneMessageStyle = clampToneStyle(prefs.getUChar("toneMsg", (uint8_t)TONE_STYLE_LOUD));
  toneDirectStyle = clampToneStyle(prefs.getUChar("toneDir", (uint8_t)TONE_STYLE_LOUD));
  if (!prefs.getBool("toneLoudV60", false)) {
    if (toneMessageStyle == TONE_STYLE_BWOMP) toneMessageStyle = TONE_STYLE_LOUD;
    if (toneDirectStyle == TONE_STYLE_DOUBLE) toneDirectStyle = TONE_STYLE_LOUD;
    prefs.putUChar("toneMsg", (uint8_t)toneMessageStyle);
    prefs.putUChar("toneDir", (uint8_t)toneDirectStyle);
    prefs.putBool("toneLoudV60", true);
  }
  if (!prefs.getBool("toneGainV64", false)) {
    toneVolumePercent = 100;
    toneBoostLevel = 5;
    toneMessageStyle = TONE_STYLE_LOUD;
    toneDirectStyle = TONE_STYLE_LOUD;
    prefs.putUChar("toneVol", toneVolumePercent);
    prefs.putUChar("toneBoost", toneBoostLevel);
    prefs.putUChar("toneMsg", (uint8_t)toneMessageStyle);
    prefs.putUChar("toneDir", (uint8_t)toneDirectStyle);
    prefs.putBool("toneGainV64", true);
  }
  if (!prefs.getBool("tonePathV68", false)) {
    toneVolumePercent = 100;
    toneBoostLevel = 5;
    toneCodecProfile = TONE_CODEC_PROFILE_DRIVER;
    prefs.putUChar("toneVol", toneVolumePercent);
    prefs.putUChar("toneBoost", toneBoostLevel);
    prefs.putUChar("toneProfile", toneCodecProfile);
    prefs.putBool("tonePathV68", true);
  }
#if ONE_TIME_CHANNEL_PROVISION
  channelProvisionDone = prefs.getBool(CHANNEL_PROVISION_PREF_KEY, false);
  channelProvisionStep = 0;
  nextChannelProvisionMs = millis() + CHANNEL_PROVISION_BOOT_DELAY_MS;
#endif
  localBattery.calibrationOffsetTenths = prefs.getShort("batTrim10", 0);
  localBattery.calibrationOffsetTenths = constrain(localBattery.calibrationOffsetTenths, -120, 120);
  localBattery.calibrationOffsetMv = 0;
  loadInterfaceSettings();
  applyFontMode();
  Serial.println("[boot] init screen");
  initScreen();
  Serial.println("[boot] init audio");
  initToneAudio();
  Serial.println("[boot] init sd");
  initSdStorage();

  char bootLine[96];
  snprintf(bootLine, sizeof(bootLine), "[boot] %s starting\n", interfaceDeviceName);
  appendLine(eventLog, LOG_SIZE, bootLine);
#if ONE_TIME_CHANNEL_PROVISION
  appendLine(eventLog, LOG_SIZE, channelProvisionDone ? "[provision] already completed\n" : "[provision] queued for Public/Family channel setup\n");
#endif
  copyPreferenceString("wifiSsid", wifiLocalSsid, sizeof(wifiLocalSsid), "");
  copyPreferenceString("wifiPass", wifiLocalPass, sizeof(wifiLocalPass), "");
  if (taWifiSsid) lv_textarea_set_text(taWifiSsid, wifiLocalSsid);
  if (taWifiPass) lv_textarea_set_text(taWifiPass, wifiLocalPass);
  if (swWifiApMode) {
    if (wifiApMode) lv_obj_add_state(swWifiApMode, LV_STATE_CHECKED);
    else lv_obj_clear_state(swWifiApMode, LV_STATE_CHECKED);
  }

  Serial.println("[boot] routes");
  registerWebRoutes();
  Serial.println("[boot] wifi");
  startWifi();
  Serial.println("[boot] wifi done");

  char line[128];
  String wifiIp = wifiEnabled ? (wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) : String("0.0.0.0");
  snprintf(line, sizeof(line), "[boot] WiFi %s %s at %s\n",
           wifiEnabled ? "on" : "off",
           wifiApMode ? "AP" : "Local",
           wifiIp.c_str());
  appendLine(eventLog, LOG_SIZE, line);
  Serial.println("[boot] config request");
  sendConfigRequest();
  Serial.println("[boot] setup complete");
}

void loop() {
  pollLocalGps();
  serviceUsbSerialCommands();
  pollLoRa();
  serviceRadioApiKeepalive();
  serviceConfigRequests();
  serviceOneTimeChannelProvision();
  if (wifiEnabled) server.handleClient();
  serviceSdDownload();
  if (otaRestartPending && (int32_t)(millis() - otaRestartAtMs) >= 0) {
    Serial.println("[ota] restarting");
    delay(100);
    ESP.restart();
  }
  serviceScreen();
  printSerialDiagnostics();
  delay(2);
}

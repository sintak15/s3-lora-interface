#include <Arduino.h>
#include <limits.h>
#include <atomic>
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
#include "src/WebMapAssets.h"
#include "src/Reliability.h"
#include "src/FieldStatus.h"
#include "src/Max17048.h"
#include "src/SampleVariation.h"
#include "src/LogRotation.h"
#include "src/ChatText.h"
#include "src/MapTileCache.h"
#include "src/TileUpload.h"
#include "src/MapNavigation.h"
#include "src/MapCoverage.h"
#include "src/BuildIdentity.h"
#include "src/GpsUbx.h"
#include "src/GpsConfig.h"
#include "src/GpsRelay.h"
#include "src/DiagnosticText.h"

#ifdef FIRMWARE_RUN_TESTS
#include "src/tests/ReliabilityTests.h"
#include "src/tests/FieldStatusTests.h"
#include "src/tests/Max17048Tests.h"
#include "src/tests/SampleVariationTests.h"
#include "src/tests/LogRotationTests.h"
#include "src/tests/ChatTextTests.h"
#include "src/tests/MapTileCacheTests.h"
#include "src/tests/TileUploadTests.h"
#include "src/tests/MapNavigationTests.h"
#include "src/tests/MapCoverageTests.h"
#include "src/tests/GpsUbxTests.h"
#include "src/tests/GpsConfigTests.h"
#include "src/tests/GpsRelayTests.h"
#include "src/tests/DiagnosticTextTests.h"
#include "src/tests/NotificationSoundTests.h"
#endif

#include "src/firmware/State.h"
#include "src/firmware/Declarations.h"

#include "src/firmware/Runtime.inc"
#include "src/firmware/Storage.inc"
#include "src/firmware/Gps.inc"
#include "src/firmware/Battery.inc"
#include "src/firmware/PowerLogStorage.inc"
#include "src/firmware/BatteryCycle.inc"
#include "src/firmware/Network.inc"
#include "src/firmware/DisplaySettings.inc"
#include "src/firmware/Audio.inc"
#include "src/firmware/Ui.inc"
#include "src/firmware/Map.inc"
#include "src/firmware/UsbSdExport.inc"
#include "src/firmware/UsbTiles.inc"
#include "src/firmware/UiRefresh.inc"
#include "src/firmware/Radio.inc"
#include "src/firmware/GpsRelay.inc"
#include "src/firmware/Http.inc"

void setup() {
  // Hold a complete encoded tile chunk while the main loop services radio/UI work.
  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  delay(3000);
  Serial.println("[boot] serial ready");
  printFirmwareIdentity();
  initSdMutex();
  initSdAppendWorker();
  allocateRuntimeBuffers();
  initMapRenderWorker();
  nextMeshPacketId = esp_random();
  if (nextMeshPacketId == 0) nextMeshPacketId = 1;
  SerialLoRa.setRxBufferSize(4096);
  SerialLoRa.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  SerialGPS.setRxBufferSize(4096);
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("[boot] prefs");
  prefs.begin("s3-lora", false);
  gpsRelay.enabled = prefs.getBool("gpsShare", false);
  mapView.zoom = MapNavigation::clampZoom(prefs.getUChar("mapZoom", MAP_TILE_MAX_ZOOM));
  wifiApMode = prefs.getBool("wifiApMode", true);
  readableTextMode = prefs.getBool("readableText", false);
  nodeListViewMode = prefs.getBool("nodeCompact", true) ? NODE_LIST_VIEW_COMPACT : NODE_LIST_VIEW_DETAILED;
  toneVolumePercent = constrain((int)prefs.getUChar("toneVol", 50), 0, 100);
  toneMessageStyle = clampToneStyle(prefs.getUChar("toneMsg", (uint8_t)TONE_STYLE_RISING_BOOP));
  toneDirectStyle = clampToneStyle(prefs.getUChar("toneDir", (uint8_t)TONE_STYLE_RISING_BOOP));
  if (!prefs.getBool("toneCleanV93", false)) {
    // Retire the old forced Max gain without overwriting quieter preferences.
    toneVolumePercent = min<uint8_t>(toneVolumePercent, 50);
    prefs.putUChar("toneVol", toneVolumePercent);
    prefs.putUChar("toneBoost", 0);
    prefs.putUChar("toneProfile", 0);
    prefs.putBool("toneCleanV93", true);
  }
#if ONE_TIME_CHANNEL_PROVISION
  channelProvisionDone = prefs.getBool(CHANNEL_PROVISION_PREF_KEY, false);
  channelProvisionStep = 0;
  nextChannelProvisionMs = millis() + CHANNEL_PROVISION_BOOT_DELAY_MS;
#endif
  // Retire the voltage-derived percentage trim; the fuel gauge owns SOC.
  if (prefs.isKey("batTrim10")) prefs.remove("batTrim10");
  localBattery.savedDischargeRate = prefs.getFloat("batRateV1", 0);
  if (!FieldStatus::RuntimeEstimate::validRate(localBattery.savedDischargeRate)) localBattery.savedDischargeRate = 0;
  batteryRuntime.restore(localBattery.savedDischargeRate);
  localBattery.runtimeSaveAttempted = localBattery.savedDischargeRate > 0;
  localBattery.lastRuntimeSaveMs = millis();
  // Continuous diagnostics replace the old manual enable/size-stop setting.
  if (prefs.isKey("batCycleOn")) prefs.remove("batCycleOn");
  batteryCycle.rotationEnabled = prefs.getBool("batLogLimit", true);
  batteryCycle.rotationPending = prefs.getBool("batLogPending", false);
  batteryCycle.restoredAgeMs = prefs.getULong64("batLogAge", 0);
  batteryCycle.rotationClock.restore(batteryCycle.restoredAgeMs, millis());
  batteryCycle.lastCheckpointMs = millis();
  batteryCycle.session = prefs.getUInt("batCycleId", 0);
  if (prefs.isKey("batCyclePhase")) prefs.remove("batCyclePhase");
  batteryCycle.boot = esp_random();
  if (!batteryCycle.session) {
    batteryCycle.session = esp_random();
    prefs.putUInt("batCycleId", batteryCycle.session);
  }
  strlcpy(batteryCycle.status, "Continuous logging; waiting for SD", sizeof(batteryCycle.status));
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
  serviceGpsRelay();
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

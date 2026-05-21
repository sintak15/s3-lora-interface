#line 1 "C:\\Users\\justi\\Documents\\Arduino\\s3-lora-interface\\constants.h"
#pragma once

#if __has_include("config/device_config.local.h")
#include "config/device_config.local.h"
#else
#include "config/device_config.example.h"
#endif

#define FIRMWARE_VERSION "v0.1.0"

#ifndef DEVICE_NAME
#define DEVICE_NAME "jjs-node"
#endif

#ifndef DEVICE_HOSTNAME
#define DEVICE_HOSTNAME DEVICE_NAME
#endif

// UART connection from this ESP32-S3 interface board to the Heltec LoRa unit.
// Cross TX/RX between boards: interface TX -> Heltec RX, interface RX -> Heltec TX.
#define LORA_RX_PIN 14
#define LORA_TX_PIN 21
#define LORA_BAUD 115200

// Optional split GPS TX input on the CYD expansion header.
// Wire GPS TX to GPIO3 and share ground with the GPS power source.
#define GPS_RX_PIN 3
#define GPS_TX_PIN -1
#define GPS_BAUD 9600

// 2.8 inch ESP32-S3 ILI9341 display and capacitive touch.
#define SCREEN_W 240
#define SCREEN_H 320
#define TFT_BL 45
#define TOUCH_ADDR 0x38
#define TOUCH_RST 18
#define TOUCH_INT 17
#define TOUCH_SDA 16
#define TOUCH_SCL 15

// Optional MAX17048 fuel gauge on the shared CYD I2C bus.
// Wire MAX17048 SDA -> GPIO16 and SCL -> GPIO15.
#define MAX17048_ADDR 0x36

// Built-in MicroSD socket on the LCDWiki 2.8" ESP32-S3 display.
// The socket uses the ESP32-S3 SDMMC/SDIO peripheral, not the TFT SPI bus.
#define SD_CLK_PIN 38
#define SD_CMD_PIN 40
#define SD_D0_PIN 39
#define SD_D1_PIN 41
#define SD_D2_PIN 48
#define SD_D3_PIN 47
#define SD_MMC_ONE_BIT false
#define SD_MMC_FREQ 20000

// Browser interface served by this board.
#ifndef INTERFACE_AP_SSID
#define INTERFACE_AP_SSID DEVICE_NAME
#endif

#ifndef INTERFACE_AP_PASS
#define INTERFACE_AP_PASS "12345678"
#endif

#ifndef WIFI_AP_CHANNEL
#define WIFI_AP_CHANNEL 6
#endif

#ifndef WEBUI_AUTH_USER
#define WEBUI_AUTH_USER "sintak"
#endif

#ifndef WEBUI_AUTH_PASS
#define WEBUI_AUTH_PASS "admin"
#endif

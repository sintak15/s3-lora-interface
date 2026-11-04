#pragma once

#define DEVICE_NAME "s3-lora-interface"
#define DEVICE_HOSTNAME DEVICE_NAME
#define INTERFACE_AP_SSID DEVICE_NAME
#define INTERFACE_AP_PASS "setup1234"
#define WIFI_AP_CHANNEL 6
#define WEBUI_AUTH_USER "admin"
#define WEBUI_AUTH_PASS "setup1234"

// ATGM336H: 9600 baud NMEA, receive-only GPIO3. GPS RX may stay disconnected.
// GPS_RECEIVER_UBLOX defaults to 0; u-blox configuration controls are disabled.
// ADS1115 A0: 10k/10k divider, ADDR to GND (0x48), VDD at CYD 3V3.
// #define BATTERY_ADS1115_ADDR 0x48
// #define BATTERY_ADC_SCALE 1.0f

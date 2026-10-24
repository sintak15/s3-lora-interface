#pragma once
#include "../DiagnosticText.h"

namespace DiagnosticTextTests {
constexpr bool same(const char* a, const char* b) {
  for (size_t i = 0;; ++i) { if (a[i] != b[i]) return false; if (!a[i]) return true; }
}
constexpr bool labelsAndReadings() {
  char out[180] = {};
  return DiagnosticText::format(out, sizeof(out), "Charge rate: 3.14%/hr\nPack raw: 3.814 V") &&
         same(out, "#85BFFF Charge rate:# 3.14%/hr\n#85BFFF Pack raw:# 3.814 V");
}
constexpr bool multipleReadings() {
  char out[160] = {};
  return DiagnosticText::format(out, sizeof(out), "Known: 3  Active: 2\nMAC: F0:9E:12:34:56:78") &&
         same(out, "#85BFFF Known:# 3  #85BFFF Active:# 2\n#85BFFF MAC:# F0:9E:12:34:56:78");
}
constexpr bool literalData() {
  char out[160] = {};
  return DiagnosticText::format(out, sizeof(out), "SSID: Node #1\nNode: #FF0000 red#\nTime: 12:34") &&
         same(out, "#85BFFF SSID:# Node ##1\n#85BFFF Node:# ##FF0000 red##\n#85BFFF Time:# 12:34");
}
constexpr bool boundedOutput() {
  char out[18] = {};
  if (DiagnosticText::format(out, sizeof(out), "Voltage: 3.14 V")) return false;
  char small[2] = {};
  return !DiagnosticText::format(small, sizeof(small), "#") && small[0] == 0 &&
         !DiagnosticText::format(nullptr, 0, "Voltage: 3.14 V");
}
static_assert(labelsAndReadings(), "Readings must remain outside label color commands");
static_assert(multipleReadings(), "Inline categories and MAC/time colons must stay distinct");
static_assert(literalData(), "Node names and SSIDs must not inject LVGL colors");
static_assert(boundedOutput(), "Small buffers must fail safely without dangling commands");
} // namespace DiagnosticTextTests

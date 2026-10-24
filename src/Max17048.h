#pragma once
#include <stdint.h>

namespace Max17048 {
// MAX17048 register units, without voltage-derived SOC corrections.
constexpr uint32_t millivolts(uint16_t raw) { return (uint32_t(raw) * 5 + 32) / 64; }
constexpr float soc(uint16_t raw) {
  return raw > 25600 ? 100.0f : raw / 256.0f;
}
constexpr float rate(uint16_t raw) {
  return (raw & 0x8000 ? int32_t(raw) - 65536 : int32_t(raw)) * 0.208f;
}
constexpr const char* trend(bool valid, float rate) {
  return !valid || !(rate == rate) ? "unavailable" :
         rate > 0 ? "rising" : rate < 0 ? "falling" : "steady";
}
constexpr const char* trendLabel(bool valid, float rate) {
  return !valid || !(rate == rate) ? "Unavailable" :
         rate > 0 ? "+" : rate < 0 ? "-" : "=";
}
} // namespace Max17048

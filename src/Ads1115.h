#pragma once
#include <stdint.h>
namespace Ads1115 {
// Single-shot AIN0 to GND, +/-4.096 V, 128 samples/s, comparator disabled.
constexpr uint16_t CONFIG = 0xC383;
constexpr uint16_t channelConfig(uint8_t channel) { return CONFIG + ((uint16_t)(channel & 3) << 12); }
constexpr bool configured(uint16_t config, uint8_t channel = 0) { return channel < 4 && (config & 0x7fff) == (channelConfig(channel) & 0x7fff); }
constexpr bool ready(uint16_t config) { return (config & 0x8000) != 0; }
constexpr bool usable(uint16_t raw) { return raw > 0 && raw < 0x7ff8; }
constexpr bool auxiliaryUsable(uint16_t raw) { return raw < 0x7ff8; }
constexpr uint32_t inputMillivolts(uint16_t raw) { return (raw + 4UL) / 8; }
constexpr uint8_t relativeLight(uint32_t mv) { return mv >= 3300 ? 100 : (mv * 100 + 1650) / 3300; }
constexpr uint32_t cellMillivolts(uint16_t raw, uint32_t top, uint32_t bottom, float scale) {
  return usable(raw) && bottom ? (uint32_t)(raw * 0.125f * ((float)top + bottom) / bottom * scale + 0.5f) : 0;
}
}

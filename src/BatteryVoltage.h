#pragma once
#include <stdint.h>
namespace BatteryVoltage {
constexpr uint32_t cellMillivolts(uint32_t adc, uint32_t top, uint32_t bottom, float scale = 1.0f) {
  return bottom ? (uint32_t)(adc * ((float)top + bottom) / bottom * scale + 0.5f) : 0;
}
constexpr bool valid(uint32_t mv) { return mv >= 2500 && mv <= 4350; }
constexpr int estimatedPercent(uint32_t mv) {
  // Generic single-cell Li-ion curve. Load, temperature and charging affect it.
  const uint16_t volts[] = {3300, 3500, 3600, 3700, 3750, 3800, 3850, 3900, 4000, 4100, 4200};
  if (mv <= volts[0]) return 0;
  for (unsigned i = 1; i < sizeof(volts) / sizeof(volts[0]); ++i)
    if (mv < volts[i]) return (i - 1) * 10 + (mv - volts[i - 1]) * 10 / (volts[i] - volts[i - 1]);
  return 100;
}
}

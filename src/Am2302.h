#pragma once
#include <stdint.h>
namespace Am2302 {
struct Reading { bool valid = false; int16_t temperatureTenths = 0; uint16_t humidityTenths = 0; };
constexpr Reading decode(const uint8_t* data) {
  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return {};
  uint16_t humidity = (uint16_t)data[0] << 8 | data[1];
  int16_t magnitude = ((data[2] & 0x7f) << 8) | data[3];
  int16_t temperature = (data[2] & 0x80) ? -magnitude : magnitude;
  return humidity <= 1000 && temperature >= -400 && temperature <= 800 ? Reading{true, temperature, humidity} : Reading{};
}
// Pulse durations in microseconds. Handshake must precede exactly forty bits.
struct Capture {
  uint8_t bytes[5]{};
  uint8_t bits = 0, phase = 0;
  uint32_t edgeAt = 0;
  bool done = false, failed = false;
  constexpr void reset(uint32_t now) { for (auto& b : bytes) b = 0; bits = phase = 0; edgeAt = now; done = failed = false; }
  constexpr __attribute__((always_inline)) void edge(bool high, uint32_t now) {
    uint32_t duration = now - edgeAt; edgeAt = now;
    if (done || failed) return;
    if (phase == 0) { if (high && duration >= 60 && duration <= 120) phase = 1; return; }
    if (phase == 1) { if (high || duration < 60 || duration > 120) { failed = true; return; } phase = 2; return; }
    if (phase == 2) { if (!high || duration < 30 || duration > 90) { failed = true; return; } phase = 3; return; }
    if (high || duration < 15 || duration > 100) { failed = true; return; }
    bytes[bits / 8] = (uint8_t)((bytes[bits / 8] << 1) | (duration > 50));
    done = ++bits == 40; phase = 2;
  }
};
}

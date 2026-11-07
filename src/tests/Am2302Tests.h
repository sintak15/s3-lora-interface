#pragma once
#include "../Am2302.h"
namespace Am2302Tests {
constexpr uint8_t warm[] = {2, 140, 1, 95, 238};
constexpr uint8_t cold[] = {0, 0, 128, 101, 229};
constexpr uint8_t corrupt[] = {2, 140, 1, 95, 0};
constexpr uint8_t excessive[] = {3, 233, 0, 0, 236};
static_assert(Am2302::decode(warm).valid && Am2302::decode(warm).temperatureTenths == 351 && Am2302::decode(warm).humidityTenths == 652, "Decode 35.1 C and 65.2% RH");
static_assert(Am2302::decode(cold).valid && Am2302::decode(cold).temperatureTenths == -101, "Signed temperature");
static_assert(!Am2302::decode(corrupt).valid && !Am2302::decode(excessive).valid, "Reject checksum/range errors");
constexpr bool pulses() {
  Am2302::Capture capture; uint32_t time = 0xffffff00UL;
  capture.reset(time);
  capture.edge(true, time += 5); // host release
  capture.edge(false, time += 30);
  capture.edge(true, time += 80); capture.edge(false, time += 80);
  for (unsigned bit = 0; bit < 40; ++bit) {
    capture.edge(true, time += 50);
    capture.edge(false, time += (warm[bit / 8] & (0x80 >> (bit % 8))) ? 70 : 26);
    if (bit < 39 && capture.done) return false;
  }
  return capture.done && !capture.failed && Am2302::decode(capture.bytes).valid &&
         Am2302::decode(capture.bytes).humidityTenths == 652;
}
static_assert(pulses(), "Handshake, 40 bits, widths and timer rollover");
constexpr bool truncated() {
  Am2302::Capture capture;
  capture.reset(0); capture.edge(false, 10); capture.edge(true, 90); capture.edge(false, 170);
  capture.edge(true, 220); capture.edge(false, 246);
  if (capture.done || capture.failed) return false;
  capture.edge(true, 500); // invalid data-low pulse
  return capture.failed && !capture.done;
}
static_assert(truncated(), "Short/noisy frames must not publish a measurement");
}

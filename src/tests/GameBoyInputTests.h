#ifndef CYD_GAMEBOY_INPUT_TESTS_H
#define CYD_GAMEBOY_INPUT_TESTS_H
#include "../games/GameBoyInput.h"
namespace GameBoyInputTests {
constexpr void point(uint8_t* p, unsigned index, int x, int y, unsigned event = 0) {
  unsigned at = 1 + index * 6; p[at] = (event << 6) | ((x >> 8) & 15); p[at + 1] = x & 255;
  p[at + 2] = (y >> 8) & 15; p[at + 3] = y & 255;
}
constexpr bool everyKey() {
  for (const auto& key : GameBoyInput::keys) {
    uint8_t packet[13] = {1}; point(packet, 0, 13 + key.x + key.w / 2, 47 + key.y + key.h / 2);
    if (GameBoyInput::decode(packet, sizeof(packet), 13, 47) != key.mask) return false;
  }
  return true;
}
constexpr bool multiTouch() {
  uint8_t packet[13] = {2}; point(packet, 0, 60, 60 + 178); point(packet, 1, 200, 60 + 180, 2);
  if (GameBoyInput::decode(packet, sizeof(packet), 0, 60) != (GAMEBOY_UP | GAMEBOY_A)) return false;
  packet[1] |= 0x40;
  if (GameBoyInput::decode(packet, sizeof(packet), 0, 60) != GAMEBOY_A) return false;
  if (GameBoyInput::decode(packet, 10, 0, 60)) return false;
  packet[0] = 0; return GameBoyInput::decode(packet, sizeof(packet), 0, 60) == 0;
}
constexpr bool viewport() {
  if (GameBoyInput::sourceX(0) != 0 || GameBoyInput::sourceX(239) != 159 || GameBoyInput::firstRow(0) != 0 || GameBoyInput::firstRow(144) != 160) return false;
  for (unsigned line = 0; line < 144; ++line) if (GameBoyInput::firstRow(line + 1) <= GameBoyInput::firstRow(line) || GameBoyInput::firstRow(line + 1) > 160) return false;
  for (const auto& key : GameBoyInput::keys) if (key.x < 0 || key.x + key.w > 240 || key.y < 160 || key.y + key.h > 260) return false;
  return true;
}
static_assert(everyKey(), "all eight FT6336 touch targets use native Peanut-GB masks");
static_assert(multiTouch(), "two contacts, individual lift, truncated packets and release");
static_assert(viewport(), "scaled video covers every line and leaves controls in bounds");
static_assert(GameBoyInput::decode(nullptr, 0, 0, 60) == 0, "missing packet releases keys");
}
#endif

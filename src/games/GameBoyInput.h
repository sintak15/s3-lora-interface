#ifndef CYD_GAMEBOY_INPUT_H
#define CYD_GAMEBOY_INPUT_H
#include <stdint.h>
#include <stddef.h>
#include "PeanutGbCore.h"
namespace GameBoyInput {
static constexpr unsigned VIDEO_W = 240, VIDEO_H = 160;
constexpr unsigned sourceX(unsigned x) { return x * 160 / VIDEO_W; }
constexpr unsigned firstRow(unsigned line) { return (line * VIDEO_H + 143) / 144; }
struct Key { const char* name; int x, y, w, h; uint8_t mask; };
static constexpr Key keys[] = {{"^",42,162,38,32,GAMEBOY_UP},{"<",2,195,38,32,GAMEBOY_LEFT},{">",82,195,38,32,GAMEBOY_RIGHT},{"v",42,228,38,32,GAMEBOY_DOWN},
  {"B",126,179,48,44,GAMEBOY_B},{"A",182,165,56,44,GAMEBOY_A},{"Select",126,228,48,30,GAMEBOY_SELECT},{"Start",182,228,56,30,GAMEBOY_START}};
constexpr uint8_t at(int x, int y) {
  uint8_t mask = 0;
  for (const auto& key : keys) if (x >= key.x && x < key.x + key.w && y >= key.y && y < key.y + key.h) mask |= key.mask;
  return mask;
}
constexpr uint8_t decode(const uint8_t* points, size_t bytes, int originX, int originY) {
  if (!points || !bytes) return 0;
  unsigned count = points[0] & 15;
  if (!count || count > 2 || bytes < 1 + (count - 1) * 6 + 4) return 0;
  uint8_t mask = 0;
  for (unsigned i = 0; i < count; ++i) {
    unsigned offset = 1 + i * 6;
    unsigned event = points[offset] >> 6;
    if (event == 1 || event == 3) continue;
    int x = ((points[offset] & 15) << 8) | points[offset + 1];
    int y = ((points[offset + 2] & 15) << 8) | points[offset + 3];
    mask |= at(x - originX, y - originY);
  }
  return mask;
}
}

#endif

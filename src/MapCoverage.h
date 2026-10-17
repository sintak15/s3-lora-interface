#pragma once
#include <cmath>
#include <stdint.h>

namespace MapCoverage {
constexpr int minZoom = 10, maxZoom = 14, levels = 5;
constexpr unsigned count(uint8_t mask) {
  unsigned n = 0;
  for (; mask; mask >>= 1) n += mask & 1U;
  return n;
}
struct Cell { int32_t x = 0, y = 0; };
struct Footprint {
  Cell cells[4]{};
  uint8_t count = 0, centerMask = 0;
};
inline Footprint footprint(double centerX, double centerY, int zoom, int width, int height) {
  Footprint out;
  if (zoom < minZoom || zoom > maxZoom || width <= 0 || width > 256 || height <= 0 || height > 256) return out;
  int worldTiles = 1 << zoom;
  int worldPixels = worldTiles * 256;
  centerX = fmax(0.0, fmin(worldPixels - 0.000001, centerX));
  centerY = fmax(0.0, fmin(worldPixels - 0.000001, centerY));
  int startX = (int)lround(centerX) - width / 2, startY = (int)lround(centerY) - height / 2;
  int left = (int)fmax(0.0, floor(startX / 256.0)), top = (int)fmax(0.0, floor(startY / 256.0));
  int right = (int)fmin(worldTiles - 1, floor((startX + width - 1) / 256.0));
  int bottom = (int)fmin(worldTiles - 1, floor((startY + height - 1) / 256.0));
  int cx = (int)floor(centerX / 256), cy = (int)floor(centerY / 256);
  for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
    if (out.count == 4) return {};
    if (x == cx && y == cy) out.centerMask = (uint8_t)(1U << out.count);
    out.cells[out.count++] = {x, y};
  }
  return out;
}
inline bool same(const Footprint& a, const Footprint& b) {
  if (a.count != b.count || a.centerMask != b.centerMask) return false;
  for (unsigned i = 0; i < a.count; ++i) if (a.cells[i].x != b.cells[i].x || a.cells[i].y != b.cells[i].y) return false;
  return true;
}
struct Level {
  Footprint area;
  uint8_t validMask = 0, invalidMask = 0;
  bool checked = false, readError = false;
  constexpr unsigned available() const { return count(validMask); }
  constexpr unsigned invalid() const { return count(invalidMask); }
  constexpr bool center() const { return checked && !readError && (validMask & area.centerMask); }
  constexpr bool complete() const { return center() && area.count && available() == area.count; }
};
struct Profile {
  Level zooms[levels];
  bool ready = false;
  uint32_t generation = 0, checkedMs = 0;
};
inline int choose(const Profile& profile, int requested) {
  if (!profile.ready) return -1;
  requested = requested > maxZoom ? maxZoom : requested;
  for (int z = requested; z >= minZoom; --z) if (profile.zooms[z-minZoom].complete()) return z;
  for (int z = requested; z >= minZoom; --z) if (profile.zooms[z-minZoom].center()) return z;
  return -1;
}
inline const char* status(const Level& level) {
  if (!level.checked) return "Unchecked";
  if (level.readError) return "Read error";
  if (level.complete()) return "Ready";
  if (level.available()) return "Partial";
  return level.invalid() ? "Unreadable" : "Missing";
}
} // namespace MapCoverage

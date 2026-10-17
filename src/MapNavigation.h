#pragma once
#include <cmath>
#include <stdint.h>

namespace MapNavigation {
constexpr double pi = 3.14159265358979323846;
constexpr double maxLatitude = 85.05112878;
constexpr int clampZoom(int zoom) { return zoom < 10 ? 10 : (zoom > 14 ? 14 : zoom); }
constexpr bool fresh(bool valid, uint32_t updated, uint32_t now) {
  return valid && (uint32_t)(now - updated) <= 30000;
}
struct Point { double lat, lon; };
inline double clampLatitude(double lat) { return fmax(-maxLatitude, fmin(maxLatitude, lat)); }
inline Point pan(Point center, int zoom, int dx, int dy) {
  double world = 256.0 * (1U << clampZoom(zoom));
  double x = (center.lon + 180.0) / 360.0 * world - dx;
  double radians = clampLatitude(center.lat) * pi / 180.0;
  double y = (1.0 - asinh(tan(radians)) / pi) / 2.0 * world - dy;
  x = fmod(fmod(x, world) + world, world);
  y = fmax(0.0, fmin(world, y));
  return {clampLatitude(atan(sinh(pi * (1.0 - 2.0 * y / world))) * 180.0 / pi), x / world * 360.0 - 180.0};
}
struct Scale { double meters; int pixels; unsigned feet; double miles; };
inline Scale scale(double lat, int zoom, int maxPixels = 80) {
  double metersPerPixel = cos(clampLatitude(lat) * pi / 180.0) * (2.0 * pi * 6378137.0) / (256.0 * (1U << clampZoom(zoom)));
  double limit = metersPerPixel * maxPixels;
  if (limit >= 1609.344 / 4) {
    double miles = limit / 1609.344;
    double distance;
    if (miles < 1) distance = miles >= 0.5 ? 0.5 : 0.25;
    else {
      double unit = pow(10.0, floor(log10(miles)));
      distance = unit * (miles >= 5 * unit ? 5 : (miles >= 2 * unit ? 2 : 1));
    }
    return {distance * 1609.344, (int)lround(distance * 1609.344 / metersPerPixel), 0, distance};
  }
  double feet = limit / 0.3048;
  double unit = pow(10.0, floor(log10(feet)));
  unsigned distance = (unsigned)lround(unit * (feet >= 5 * unit ? 5 : (feet >= 2 * unit ? 2 : 1)));
  return {distance * 0.3048, (int)lround(distance * 0.3048 / metersPerPixel), distance, 0};
}
struct View {
  int zoom = 14;
  bool follow = true, manual = false, hasCenter = false;
  Point center{0, 0};
  uint32_t revision = 1;
  bool setZoom(int value) {
    value = clampZoom(value);
    if (zoom == value) return false;
    zoom = value; ++revision;
    return true;
  }
  void pause() { follow = false; manual = hasCenter; ++revision; }
  void resume() { follow = true; manual = false; ++revision; }
  void panFrom(Point origin, int dx, int dy) {
    center = pan(origin, zoom, dx, dy);
    hasCenter = true; manual = true; follow = false; ++revision;
  }
};
} // namespace MapNavigation

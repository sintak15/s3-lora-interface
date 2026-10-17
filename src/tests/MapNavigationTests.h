#pragma once
#include "../MapNavigation.h"
namespace MapNavigationTests {
static_assert(MapNavigation::clampZoom(-5) == 10 && MapNavigation::clampZoom(30) == 14 && MapNavigation::clampZoom(12) == 12, "Zoom must stay inside the installed tile range");
static_assert(MapNavigation::fresh(true, 100, 30100) && !MapNavigation::fresh(true, 100, 30101) && !MapNavigation::fresh(false, 100, 100) && MapNavigation::fresh(true, 0xfffffff0U, 10), "GPS freshness must expire and survive millis wrap");
} // namespace MapNavigationTests

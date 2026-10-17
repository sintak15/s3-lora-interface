#pragma once
#include "../MapCoverage.h"
namespace MapCoverageTests {
static_assert(MapCoverage::count(0x0f) == 4 && MapCoverage::count(0) == 0, "Coverage must count actual readable tiles");
constexpr MapCoverage::Level full{{{}, 4, 1}, 15, 0, true, false};
constexpr MapCoverage::Level partial{{{}, 4, 1}, 1, 0, true, false};
constexpr MapCoverage::Level invalid{{{}, 4, 1}, 0, 1, true, false};
static_assert(full.complete() && partial.center() && !partial.complete() && invalid.invalid() == 1 && !invalid.center(), "Partial and malformed tiles must not be called complete coverage");
} // namespace MapCoverageTests

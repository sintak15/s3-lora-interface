#pragma once
#include "../Max17048.h"

static_assert(Max17048::millivolts(0xC800) == 4000, "VCELL 78.125 uV/LSB: 4 V");
static_assert(Max17048::millivolts(0) == 0, "Do not mask a zero-voltage gauge reading");
static_assert(Max17048::soc(0x3280) == 50.5f, "SOC includes the fractional byte");
static_assert(Max17048::soc(0x6500) == 100, "Saturate SOC above 100 percent");
static_assert(Max17048::soc(0) == 0, "Do not substitute voltage for zero SOC");
static_assert(Max17048::rate(0x0005) > 1.039f && Max17048::rate(0x0005) < 1.041f, "Positive CRATE");
static_assert(Max17048::rate(0xFFFB) < -1.039f && Max17048::rate(0xFFFB) > -1.041f, "Signed CRATE");
static_assert(Max17048::rate(0x8000) < -6800, "CRATE sign extension at negative limit");
static_assert(Max17048::trend(false, 10)[0] == 'u', "A failed CRATE read is unavailable");
static_assert(Max17048::trend(true, 0)[0] == 's', "Zero rate has no direction");
static_assert(Max17048::trend(true, -0.208f)[0] == 'f', "No software direction threshold");
static_assert(Max17048::trend(true, 0.208f)[0] == 'r', "No software settling window");

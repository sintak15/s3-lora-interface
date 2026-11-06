#pragma once
#include "../BatteryVoltage.h"
#include "../Ads1115.h"
static_assert(BatteryVoltage::cellMillivolts(2100, 200000, 200000) == 4200, "Divider restores cell voltage");
static_assert(BatteryVoltage::cellMillivolts(2000, 100000, 200000) == 3000, "Unequal divider");
static_assert(!BatteryVoltage::valid(0) && !BatteryVoltage::valid(5000), "Reject disconnected and boosted supply");
static_assert(BatteryVoltage::valid(2500) && BatteryVoltage::valid(4200), "Single-cell range");
static_assert(BatteryVoltage::estimatedPercent(3300) == 0, "Empty clamp");
static_assert(BatteryVoltage::estimatedPercent(4200) == 100, "Full clamp");
static_assert(BatteryVoltage::estimatedPercent(3775) == 45, "Curve interpolation");

static_assert(Ads1115::cellMillivolts(16800, 10000, 10000, 1.0f) == 4200, "ADS1115 125uV LSB and divider");
static_assert(!Ads1115::usable(0xffff) && !Ads1115::usable(0x7fff) && !Ads1115::usable(0), "Reject negative, clipped and zero conversion");
static_assert(Ads1115::configured(0x4383) && !Ads1115::ready(0x4383), "Conversion in progress");
static_assert(Ads1115::configured(0xc383) && Ads1115::ready(0xc383), "Requested conversion ready");
static_assert(!Ads1115::configured(0x8583), "Reject reset/wrong-channel configuration");

static_assert(Ads1115::channelConfig(1) == 0xd383 && Ads1115::channelConfig(2) == 0xe383, "Mux select A1/A2");
static_assert(!Ads1115::configured(0xd383, 2) && Ads1115::configured(0xe383, 2), "Never attribute another channel's conversion to light");
static_assert(Ads1115::cellMillivolts(20000, 10000, 10000, 1.0f) == 5000, "Boosted 5V divider");
static_assert(Ads1115::auxiliaryUsable(0) && !Ads1115::usable(0), "Darkness/rail off valid; empty battery not valid");
static_assert(Ads1115::relativeLight(0) == 0 && Ads1115::relativeLight(1650) == 50 && Ads1115::relativeLight(3350) == 100, "Relative sensor signal range");

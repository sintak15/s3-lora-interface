#pragma once
#include <stddef.h>
#include <stdint.h>

// Keep the stored style IDs and legacy API aliases stable across updates.
enum ToneStyle : uint8_t {
  TONE_STYLE_BWOMP = 0,
  TONE_STYLE_CHIME = 1,
  TONE_STYLE_DOUBLE = 2,
  TONE_STYLE_PULSE = 3,
  TONE_STYLE_ALERT = 4,
  TONE_STYLE_LOUD = 5,
  TONE_STYLE_SUSTAIN = 6,
  TONE_STYLE_RAW = 7,
  TONE_STYLE_POP = 8,
  TONE_STYLE_DOUBLE_POP = 9,
  TONE_STYLE_COUNT = 10
};

struct ToneSegment {
  uint16_t frequency;
  uint16_t durationMs;
  uint16_t endFrequency = 0;
};

namespace NotificationSound {
// Boops use fixed pitches; pops make one very short upward sweep. Both have
// rounded edges and preserve the amplifier settling silence before playback.
static constexpr ToneSegment soft[] = {{0,120}, {660,140}, {0,100}};
static constexpr ToneSegment doubleBoop[] = {{0,120}, {660,95}, {0,75}, {660,95}, {0,100}};
static constexpr ToneSegment twoNote[] = {{0,120}, {660,120}, {0,60}, {880,140}, {0,100}};
static constexpr ToneSegment triple[] = {{0,120}, {740,80}, {0,65}, {740,80}, {0,65}, {740,80}, {0,100}};
static constexpr ToneSegment gentle[] = {{0,120}, {880,90}, {0,70}, {660,150}, {0,100}};
static constexpr ToneSegment rising[] = {{0,120}, {523,90}, {0,35}, {659,90}, {0,35}, {784,150}, {0,100}};
static constexpr ToneSegment longBoop[] = {{0,120}, {587,320}, {0,100}};
static constexpr ToneSegment diagnostic[] = {{0,120}, {1000,300}, {0,100}};
static constexpr ToneSegment pop[] = {{0,120}, {600,32,1200}, {0,100}};
static constexpr ToneSegment doublePop[] = {{0,120}, {600,32,1200}, {0,90}, {600,32,1200}, {0,100}};

struct Preset {
  const char* name;
  const ToneSegment* segments;
  size_t count;
};
static constexpr Preset presets[] = {
  {"Soft Boop", soft, sizeof(soft)/sizeof(soft[0])},
  {"Double Boop", doubleBoop, sizeof(doubleBoop)/sizeof(doubleBoop[0])},
  {"Two Note", twoNote, sizeof(twoNote)/sizeof(twoNote[0])},
  {"Triple Boop", triple, sizeof(triple)/sizeof(triple[0])},
  {"Gentle Alert", gentle, sizeof(gentle)/sizeof(gentle[0])},
  {"Rising Boop", rising, sizeof(rising)/sizeof(rising[0])},
  {"Long Boop", longBoop, sizeof(longBoop)/sizeof(longBoop[0])},
  {"Test Sine", diagnostic, sizeof(diagnostic)/sizeof(diagnostic[0])},
  {"Bubble Pop", pop, sizeof(pop)/sizeof(pop[0])},
  {"Double Pop", doublePop, sizeof(doublePop)/sizeof(doublePop[0])}
};
static_assert(sizeof(presets)/sizeof(presets[0]) == TONE_STYLE_COUNT, "Every saved style needs a preset");

constexpr const Preset& preset(ToneStyle style) {
  return presets[(uint8_t)style < TONE_STYLE_COUNT ? (uint8_t)style : 0];
}

constexpr float smoothStep(float value) {
  return value * value * (3.0f - 2.0f * value);
}

// Smooth onset followed by a gentle decay, rather than a sustained beep with
// abrupt ends. The explicit diagnostic sine retains its flat measurement body.
constexpr float envelope(uint32_t index, uint32_t total, uint32_t sampleRate, bool testSine) {
  if (total < 3 || index >= total || sampleRate < 100) return 0;
  uint32_t ramp = testSine ? sampleRate / 50 : sampleRate / 80;
  if (ramp > total / 3) ramp = total / 3;
  if (!ramp) ramp = 1;
  if (index < ramp) return smoothStep((float)index / ramp);
  if (testSine) {
    uint32_t remaining = total - 1 - index;
    return remaining < ramp ? smoothStep((float)remaining / ramp) : 1.0f;
  }
  return smoothStep((float)(total - 1 - index) / (total - 1 - ramp));
}
} // namespace NotificationSound

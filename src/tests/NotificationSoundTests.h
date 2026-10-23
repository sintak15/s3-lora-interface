#pragma once
#include "../NotificationSound.h"
#include <initializer_list>

namespace NotificationSoundTests {
using namespace NotificationSound;

static_assert(TONE_STYLE_SOFT_BOOP == 0 && TONE_STYLE_DOUBLE_BOOP == 1 && TONE_STYLE_TWO_NOTE == 2 &&
              TONE_STYLE_TRIPLE_BOOP == 3 && TONE_STYLE_GENTLE_ALERT == 4 && TONE_STYLE_RISING_BOOP == 5 &&
              TONE_STYLE_LONG_BOOP == 6 && TONE_STYLE_TEST_SINE == 7 &&
              TONE_STYLE_BUBBLE_POP == 8 && TONE_STYLE_DOUBLE_POP == 9, "Preserve saved style IDs");

constexpr bool boundedNotifications() {
  for (uint8_t style = 0; style < TONE_STYLE_COUNT; ++style) {
    const auto& sound = preset((ToneStyle)style);
    if (!sound.name || !sound.name[0] || sound.count < 3) return false;
    if (sound.segments[0].frequency || sound.segments[0].durationMs < 120) return false;
    if (sound.segments[sound.count - 1].frequency) return false;
    uint32_t duration = 0, audible = 0;
    for (size_t i = 0; i < sound.count; ++i) {
      const auto& segment = sound.segments[i];
      if (!segment.durationMs) return false;
      duration += segment.durationMs;
      if (segment.frequency) {
        ++audible;
        if (segment.frequency < 500 || segment.frequency > 1200) return false;
        if (segment.durationMs < 25 || segment.durationMs > 350) return false;
        if (segment.endFrequency && (segment.endFrequency < 500 || segment.endFrequency > 1200)) return false;
      }
    }
    if (!audible || duration + 125 >= 750) return false; // Include the DMA silence flush.
  }
  return true;
}

constexpr bool boundedEnvelope(bool diagnostic) {
  for (uint32_t total : {512U, 1280U, 2240U, 5120U}) {
    float previous = 0;
    if (envelope(0, total, 16000, diagnostic) != 0 ||
        envelope(total - 1, total, 16000, diagnostic) != 0) return false;
    for (uint32_t index = 0; index < total; ++index) {
      float value = envelope(index, total, 16000, diagnostic);
      if (!(value >= 0 && value <= 1)) return false;
      if (value - previous > 0.01f || previous - value > 0.01f) return false;
      previous = value;
    }
  }
  return true;
}

static_assert(boundedNotifications(), "Notification notes must be short, audible and bounded");
static_assert(boundedEnvelope(false), "Boops must have smooth, bounded onsets and endings");
static_assert(boundedEnvelope(true), "Diagnostic sine must retain smooth, bounded endpoints");
static_assert(envelope(1120, 2240, 16000, false) > envelope(2000, 2240, 16000, false),
              "Notification notes must decay rather than sustain");
static_assert(envelope(1120, 2240, 16000, true) == 1, "Diagnostic sine must retain a steady body");
static_assert(envelope(1, 1, 16000, false) == 0 && envelope(20, 20, 16000, false) == 0,
              "Empty, tiny or completed notes are silent");
} // namespace NotificationSoundTests

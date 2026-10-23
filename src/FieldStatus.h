#pragma once
#include <stdint.h>

namespace FieldStatus {
enum class Delivery : uint8_t { Queued, Sent, Acknowledged, Failed, Unconfirmed };
constexpr uint32_t ACK_WAIT_MS = 120000;
constexpr Delivery visibleDelivery(Delivery state, uint32_t sent, uint32_t now) {
  return (state == Delivery::Queued || state == Delivery::Sent) &&
         (uint32_t)(now - sent) >= ACK_WAIT_MS ? Delivery::Unconfirmed : state;
}
constexpr bool retryAllowed(Delivery state, uint32_t sent, uint32_t now) {
  auto visible = visibleDelivery(state, sent, now);
  return visible == Delivery::Failed || visible == Delivery::Unconfirmed;
}
constexpr Delivery queueResult(Delivery state, int result) {
  // ERRNO_OK (0) and legacy ERRNO_SHOULD_RELEASE (35) are successful
  // queue results. Queue errors use a different namespace from routing NAKs.
  return state == Delivery::Acknowledged || state == Delivery::Failed ? state :
         (result == 0 || result == 35 ? Delivery::Sent : Delivery::Failed);
}
constexpr bool acceptRouting(bool direct, uint32_t destination, uint32_t localNode,
                             uint32_t from, uint32_t to, int error) {
  if (!localNode || to != localNode) return false;
  // A NAK must originate at our radio or the intended destination.
  if (error != 0) return from == localNode || (direct && from == destination);
  return direct ? from == destination : from == localNode;
}
constexpr const char* linkState(bool seen, uint32_t last, uint32_t now) {
  return !seen ? "waiting" : (uint32_t)(now - last) >= 90000 ? "stale" :
         (uint32_t)(now - last) >= 45000 ? "quiet" : "responsive";
}

// Keep the learned discharge rate separate from session observations. A restart,
// charge or missed reading must not erase useful history or count powered-off time.
class RuntimeEstimate {
public:
  static constexpr bool validRate(float rate) { return rate >= 0.2f && rate <= 100.0f; }
  constexpr void restore(float rate) {
    learnedRate_ = validRate(rate) ? rate : 0;
    pause();
  }
  constexpr void pause(bool charging = false) {
    clearWindow();
    minutes_ = -1;
    basis_ = charging ? "charging" : "unavailable";
  }
  constexpr void sample(uint32_t now, float soc, bool valid, bool charging,
                        float gaugeRate = 0, bool gaugeRateValid = false) {
    if (!valid || charging || !(soc >= 0 && soc <= 100)) { pause(charging); return; }
    // Recalculate from fresh SOC even before the next rate observation is due.
    updateMinutes(soc);
    if (!count_) basis_ = learnedRate_ > 0 ? "previous" : "learning";
    if (count_ && (uint32_t)(now - times_[(head_ + 15) % 16]) < 60000) return;
    if (count_) {
      const unsigned previous = (head_ + 15) % 16;
      if ((uint32_t)(now - times_[previous]) > 120000 || soc > values_[previous] + 0.2f) {
        clearWindow();
      }
    }
    times_[head_] = now;
    values_[head_] = soc;
    head_ = (head_ + 1) % 16;
    if (count_ < 16) ++count_;
    const unsigned oldest = (head_ + 16 - count_) % 16;
    const uint32_t elapsed = now - times_[oldest];
    const float drop = values_[oldest] - soc;
    float candidate = 0;
    basis_ = learnedRate_ > 0 ? "previous" : "learning";
    // CRATE is itself a SOC trend, not a current measurement. Require two
    // consistent minute-spaced readings before using it for an early estimate.
    const float discharge = -gaugeRate;
    if (gaugeRateValid && validRate(discharge)) {
      gaugeSamples_ = gaugeSamples_ && discharge >= previousGaugeRate_ * 0.5f &&
                      discharge <= previousGaugeRate_ * 1.5f ? 2 : 1;
      previousGaugeRate_ = discharge;
      if (gaugeSamples_ >= 2) { candidate = discharge; basis_ = "gauge"; }
    } else {
      gaugeSamples_ = 0;
    }
    // A meaningful raw SOC drop is stronger evidence than CRATE. Use a rolling
    // window and blend changes rather than replacing the estimate abruptly.
    if (elapsed >= 180000 && drop >= 0.5f) {
      const float observed = drop * 3600000.0f / elapsed;
      if (validRate(observed)) { candidate = observed; basis_ = "current"; }
    }
    if (validRate(candidate)) {
      learnedRate_ = learnedRate_ > 0 ? learnedRate_ * 0.75f + candidate * 0.25f : candidate;
    }
    updateMinutes(soc);
  }
  constexpr int32_t minutes() const { return minutes_; }
  constexpr float percentPerHour() const { return minutes_ >= 0 ? learnedRate_ : 0; }
  constexpr float learnedPercentPerHour() const { return learnedRate_; }
  constexpr const char* basis() const { return basis_; }
private:
  constexpr void clearWindow() { count_ = head_ = gaugeSamples_ = 0; }
  constexpr void updateMinutes(float soc) {
    const float remaining = validRate(learnedRate_) ? soc * 60.0f / learnedRate_ : -1;
    minutes_ = remaining >= 0 && remaining <= 10080 ? (int32_t)remaining : -1;
  }
  uint32_t times_[16] = {};
  float values_[16] = {};
  unsigned count_ = 0, head_ = 0;
  int32_t minutes_ = -1;
  float learnedRate_ = 0, previousGaugeRate_ = 0;
  unsigned gaugeSamples_ = 0;
  const char* basis_ = "unavailable";
};

constexpr int32_t roundedRuntimeMinutes(int32_t minutes) {
  return minutes <= 0 ? minutes : minutes < 5 ? 5 : ((minutes + 2) / 5) * 5;
}

constexpr bool runtimeCheckpointDue(float rate, float saved, bool attempted, uint32_t last, uint32_t now) {
  if (!RuntimeEstimate::validRate(rate)) return false;
  const bool haveSaved = RuntimeEstimate::validRate(saved);
  if ((attempted || haveSaved) && (uint32_t)(now - last) < (haveSaved ? 900000UL : 60000UL)) return false;
  const float threshold = saved * 0.05f > 0.5f ? saved * 0.05f : 0.5f;
  const float difference = rate > saved ? rate - saved : saved - rate;
  return !haveSaved || difference >= threshold;
}
} // namespace FieldStatus

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

// Estimate from raw fuel-gauge SOC over a rolling 10-15 minute window.
// Never infer runtime from a noisy voltage-based percentage.
class RuntimeEstimate {
public:
  constexpr void reset() { count_ = head_ = 0; minutes_ = -1; rate_ = 0; }
  constexpr void sample(uint32_t now, float soc, bool valid, bool charging) {
    if (!valid || charging || !(soc >= 0 && soc <= 100)) { reset(); return; }
    if (count_ && (uint32_t)(now - times_[(head_ + 15) % 16]) < 60000) return;
    if (count_) {
      const unsigned previous = (head_ + 15) % 16;
      if ((uint32_t)(now - times_[previous]) > 120000 || soc > values_[previous] + 0.2f) reset();
    }
    times_[head_] = now;
    values_[head_] = soc;
    head_ = (head_ + 1) % 16;
    if (count_ < 16) ++count_;
    const unsigned oldest = (head_ + 16 - count_) % 16;
    const uint32_t elapsed = now - times_[oldest];
    const float drop = values_[oldest] - soc;
    minutes_ = -1;
    rate_ = 0;
    if (elapsed < 600000 || drop < 0.5f) return;
    const float rate = drop * 3600000.0f / elapsed;
    const float minutes = soc * 60.0f / rate;
    if (!(rate >= 0.2f && rate <= 100 && minutes >= 0 && minutes <= 10080)) return;
    rate_ = rate;
    minutes_ = (int32_t)minutes;
  }
  constexpr int32_t minutes() const { return minutes_; }
  constexpr float percentPerHour() const { return rate_; }
private:
  uint32_t times_[16] = {};
  float values_[16] = {};
  unsigned count_ = 0, head_ = 0;
  int32_t minutes_ = -1;
  float rate_ = 0;
};
} // namespace FieldStatus

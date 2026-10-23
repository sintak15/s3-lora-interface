#pragma once
#include "../FieldStatus.h"
#include <limits>

namespace FieldStatusTests {
using namespace FieldStatus;
static_assert(visibleDelivery(Delivery::Queued, 10, 120009) == Delivery::Queued, "Do not expire early");
static_assert(visibleDelivery(Delivery::Sent, 10, 120010) == Delivery::Unconfirmed, "No ACK is unconfirmed");
static_assert(visibleDelivery(Delivery::Sent, UINT32_MAX - 100, 120000) == Delivery::Unconfirmed, "ACK timeout wraps");
static_assert(!retryAllowed(Delivery::Acknowledged, 0, 999999), "Do not retry acknowledged messages");
static_assert(!retryAllowed(Delivery::Queued, 0, 1000), "Do not retry pending messages");
static_assert(retryAllowed(Delivery::Failed, 0, 1), "Allow failed retries");
static_assert(queueResult(Delivery::Queued, 0) == Delivery::Sent, "Queue acceptance is not delivery");
static_assert(queueResult(Delivery::Queued, 35) == Delivery::Sent, "Legacy successful release code");
static_assert(queueResult(Delivery::Queued, 32) == Delivery::Failed, "Queue full rejects send");
static_assert(queueResult(Delivery::Acknowledged, 32) == Delivery::Acknowledged, "Late queue error cannot undo ACK");
static_assert(queueResult(Delivery::Failed, 0) == Delivery::Failed, "Late queue acceptance cannot undo failure");
static_assert(linkState(true, UINT32_MAX - 1000, 90000)[0] == 's', "Health age wraps");
static_assert(linkState(false, 0, 0)[0] == 'w', "No decoded response means waiting");

static_assert(acceptRouting(true, 2, 1, 2, 1, 0), "Direct ACK must match destination");
static_assert(!acceptRouting(true, 2, 1, 1, 1, 0), "Local echo is not destination ACK");
static_assert(!acceptRouting(true, 2, 1, 3, 1, 0), "Unrelated node cannot acknowledge direct TX");
static_assert(!acceptRouting(true, 2, 1, 2, 3, 0), "ACK must address our node");
static_assert(acceptRouting(false, 0xffffffff, 1, 1, 1, 0), "Local radio synthesizes mesh ACK");
static_assert(acceptRouting(true, 2, 1, 1, 1, 3), "Local timeout must fail TX");
static_assert(!acceptRouting(true, 2, 1, 3, 1, 3), "Unrelated routing error must not fail TX");

constexpr bool near(float a, float b) { return a >= b - 0.05f && a <= b + 0.05f; }
constexpr bool runtimeLearnsFromSoc() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 2; ++i) estimate.sample(i * 60000, 80.0f - i * 0.25f, true, false);
  if (estimate.minutes() != -1) return false;
  estimate.sample(180000, 79.25f, true, false);
  return estimate.minutes() == 317 && near(estimate.percentPerHour(), 15) && estimate.basis()[0] == 'c';
}
constexpr bool runtimeSurvivesRebootAndCharge() {
  RuntimeEstimate estimate;
  estimate.restore(6);
  estimate.sample(0, 60, true, false);
  if (estimate.minutes() != 600 || estimate.basis()[0] != 'p') return false;
  estimate.sample(1000, 60, true, true);
  if (estimate.minutes() != -1 || estimate.learnedPercentPerHour() != 6) return false;
  estimate.sample(2000, 90, true, false);
  if (estimate.minutes() != 900) return false;
  RuntimeEstimate reboot;
  reboot.restore(estimate.learnedPercentPerHour());
  reboot.sample(0, 30, true, false); // Fresh SOC, never a saved runtime countdown.
  return reboot.minutes() == 300 && reboot.basis()[0] == 'p';
}
constexpr bool runtimeUsesQualifiedGaugeRate() {
  RuntimeEstimate estimate;
  estimate.sample(0, 80, true, false, -12, true);
  if (estimate.minutes() != -1) return false; // One startup reading is insufficient.
  estimate.sample(59000, 79.8f, true, false, -12, true);
  if (estimate.minutes() != -1) return false;
  estimate.sample(60000, 79.8f, true, false, -12, true);
  if (estimate.minutes() != 399 || estimate.basis()[0] != 'g') return false;
  estimate.sample(61000, 79.6f, true, false, -12, true);
  return estimate.minutes() == 398; // SOC updates between rate observations.
}
constexpr bool runtimeRejectsBadData() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 16; ++i) estimate.sample(i * 60000, 80.0f, true, false);
  if (estimate.minutes() != -1) return false;
  estimate.sample(17 * 60000, 85.0f, true, false); // Disconnect/load change or charging.
  if (estimate.minutes() != -1) return false;
  estimate.sample(18 * 60000, 80.0f, false, false);
  estimate.restore(std::numeric_limits<float>::quiet_NaN());
  if (estimate.learnedPercentPerHour() != 0) return false;
  estimate.restore(1000000);
  estimate.sample(0, 50, true, false);
  return estimate.minutes() == -1;
}
constexpr bool runtimeRollingWindowAndWrap() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 30; ++i) estimate.sample(UINT32_MAX - 300000 + i * 60000, 90.0f - i, true, false);
  if (estimate.minutes() != 60) return false;
  estimate.sample(2000000, 59, true, false); // Gap clears observations, not learned rate.
  return estimate.minutes() == 59 && estimate.basis()[0] == 'p';
}
constexpr bool runtimePausesWithoutForgetting() {
  RuntimeEstimate estimate;
  estimate.restore(12);
  estimate.sample(0, 80, true, false);
  estimate.sample(1000, 80, false, false);
  if (estimate.minutes() != -1 || estimate.learnedPercentPerHour() != 12) return false;
  estimate.sample(2000, 79, true, false);
  if (estimate.minutes() != 395 || estimate.basis()[0] != 'p') return false;
  estimate.sample(600000, 81, true, false);
  return estimate.minutes() == 405 && estimate.basis()[0] == 'p';
}
constexpr bool runtimeRejectsUnstableGaugeRate() {
  RuntimeEstimate estimate;
  estimate.sample(0, 80, true, false, -12, true);
  estimate.sample(60000, 80, true, false, -90, true);
  estimate.sample(120000, 80, true, false, 12, true);
  estimate.sample(180000, 80, true, false, 0, true);
  estimate.sample(240000, 80, true, false, -12, false);
  return estimate.minutes() == -1 && estimate.learnedPercentPerHour() == 0;
}
constexpr bool runtimeAdaptsSmoothly() {
  RuntimeEstimate estimate;
  estimate.restore(6);
  for (uint32_t i = 0; i <= 3; ++i) estimate.sample(i * 60000, 80 - i * 0.5f, true, false);
  // New observed rate is 30%/hr; blend into prior 6%/hr instead of jumping.
  return near(estimate.learnedPercentPerHour(), 12) && estimate.basis()[0] == 'c';
}
static_assert(runtimeLearnsFromSoc(), "Meaningful SOC decline qualifies an early current-use estimate");
static_assert(runtimeSurvivesRebootAndCharge(), "Restore rate, use fresh SOC and retain learning during charging");
static_assert(runtimeUsesQualifiedGaugeRate(), "Gauge trend needs consistent readings; runtime uses fresh SOC");
static_assert(runtimeRejectsBadData(), "Flat, rising or invalid SOC must not predict runtime");
static_assert(runtimeRollingWindowAndWrap(), "Runtime window and timer wrap must be safe");
static_assert(runtimePausesWithoutForgetting(), "Unreliable readings must not erase discharge learning");
static_assert(runtimeRejectsUnstableGaugeRate(), "Positive, zero, unstable or missing CRATE must not seed runtime");
static_assert(runtimeAdaptsSmoothly(), "Changed load must blend into learned discharge rate");
static_assert(roundedRuntimeMinutes(-1) == -1 && roundedRuntimeMinutes(0) == 0 &&
              roundedRuntimeMinutes(2) == 5 && roundedRuntimeMinutes(128) == 130,
              "Runtime formatting must avoid false minute precision");
static_assert(runtimeCheckpointDue(12, 0, false, 0, 60000), "Save the first qualified rate promptly");
static_assert(!runtimeCheckpointDue(12, 0, true, 60000, 61000), "Throttle failed first-save retries");
static_assert(!runtimeCheckpointDue(18, 12, true, 0, 899999), "Bound repeat writes to fifteen minutes");
static_assert(runtimeCheckpointDue(18, 12, true, 0, 900000), "Save a changed rate at the next checkpoint");
static_assert(!runtimeCheckpointDue(12.1f, 12, true, 0, 900000), "Do not rewrite flash for insignificant changes");
static_assert(runtimeCheckpointDue(18, 12, true, UINT32_MAX - 500000, 400000), "Checkpoint timer wraps safely");
static_assert(!runtimeCheckpointDue(0, 12, false, 0, 900000), "Never persist an unknown discharge rate");
} // namespace FieldStatusTests

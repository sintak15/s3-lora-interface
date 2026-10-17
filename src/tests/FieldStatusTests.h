#pragma once
#include "../FieldStatus.h"

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

constexpr bool runtimeLearnsAndResets() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 9; ++i) estimate.sample(i * 60000, 80.0f - i * 0.1f, true, false);
  if (estimate.minutes() != -1) return false;
  estimate.sample(600000, 79.0f, true, false);
  if (estimate.minutes() != 790 || estimate.percentPerHour() != 6.0f) return false;
  estimate.sample(601000, 79.0f, true, true);
  return estimate.minutes() == -1;
}
constexpr bool runtimeRejectsBadData() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 16; ++i) estimate.sample(i * 60000, 80.0f, true, false);
  if (estimate.minutes() != -1) return false;
  estimate.sample(17 * 60000, 85.0f, true, false); // Disconnect/load change or charging.
  if (estimate.minutes() != -1) return false;
  estimate.sample(18 * 60000, 80.0f, false, false);
  return estimate.minutes() == -1;
}
constexpr bool runtimeRollingWindowAndWrap() {
  RuntimeEstimate estimate;
  for (uint32_t i = 0; i <= 30; ++i) estimate.sample(UINT32_MAX - 300000 + i * 60000, 90.0f - i, true, false);
  if (estimate.minutes() != 60) return false;
  estimate.sample(2000000, 59, true, false); // Long gap invalidates old estimate.
  return estimate.minutes() == -1;
}
static_assert(runtimeLearnsAndResets(), "Estimate must wait ten minutes and reset on charge");
static_assert(runtimeRejectsBadData(), "Flat, rising or invalid SOC must not predict runtime");
static_assert(runtimeRollingWindowAndWrap(), "Runtime window and timer wrap must be safe");
} // namespace FieldStatusTests

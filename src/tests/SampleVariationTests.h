#pragma once
#include "../SampleVariation.h"

constexpr bool sampleVariationChecks() {
  SampleVariation window;
  if (window.count() || window.span() || window.variance()) return false;
  window.add(3600); window.add(3620); window.add(3640);
  if (window.count() != 3 || window.minimum() != 3600 || window.maximum() != 3640 ||
      window.span() != 40 || window.mean() != 3620 || window.largestStep() != 20 ||
      window.variance() < 266.66 || window.variance() > 266.67) return false;
  window.resetWindow(); window.add(3600);
  if (window.count() != 1 || window.span() || window.variance() || window.largestStep() != 40) return false;
  window.resetWindow(); window.add(3600); window.add(3600);
  return window.largestStep() == 0 && window.variance() == 0;
}
static_assert(sampleVariationChecks(), "Sampled voltage variation, reset and boundary-step handling");

#pragma once
#include <stdint.h>

// Variation of actual samples, not an RF-noise or current measurement.
class SampleVariation {
public:
  constexpr void add(uint32_t mv) {
    if (havePrevious_) {
      uint32_t step = mv > previous_ ? mv - previous_ : previous_ - mv;
      if (step > largestStep_) largestStep_ = step;
    }
    previous_ = mv; havePrevious_ = true;
    if (!count_ || mv < min_) min_ = mv;
    if (!count_ || mv > max_) max_ = mv;
    ++count_; sum_ += mv; squares_ += uint64_t(mv) * mv;
  }
  constexpr void resetWindow() {
    count_ = min_ = max_ = largestStep_ = 0; sum_ = squares_ = 0;
  }
  constexpr uint32_t count() const { return count_; }
  constexpr uint32_t minimum() const { return min_; }
  constexpr uint32_t maximum() const { return max_; }
  constexpr uint32_t span() const { return max_ - min_; }
  constexpr uint32_t largestStep() const { return largestStep_; }
  constexpr double mean() const { return count_ ? double(sum_) / count_ : 0; }
  constexpr double variance() const {
    double value = count_ ? double(squares_) / count_ - mean() * mean() : 0;
    return value > 0 ? value : 0;
  }
private:
  uint32_t count_ = 0, min_ = 0, max_ = 0, largestStep_ = 0, previous_ = 0;
  uint64_t sum_ = 0, squares_ = 0;
  bool havePrevious_ = false;
};

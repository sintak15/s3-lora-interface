#pragma once
#include <stdint.h>

namespace PowerLog {
constexpr uint64_t WINDOW_MS = 48ULL * 60 * 60 * 1000;
constexpr uint32_t CHECKPOINT_MS = 15UL * 60 * 1000;

class RotationClock {
public:
  constexpr void restore(uint64_t elapsed, uint32_t now) { elapsed_ = elapsed; last_ = now; }
  constexpr void tick(uint32_t now) { elapsed_ += uint32_t(now - last_); last_ = now; }
  constexpr bool due(bool limited) const { return limited && elapsed_ >= WINDOW_MS; }
  constexpr uint64_t elapsed() const { return elapsed_; }
private:
  uint64_t elapsed_ = 0;
  uint32_t last_ = 0;
};

// Journal pending BEFORE removing the old archive. Rename the whole current
// directory so the readable log and CSV always move together. Clear pending
// and reset age BEFORE creating a new current directory. A retry after a power
// cut cannot mistake that new directory for the old recording and erase it.
template<class Storage>
constexpr bool rotate(Storage& storage) {
  if (!storage.pending() && !storage.setPending(true)) return false;
  if (storage.currentExists()) {
    if (!storage.removePrevious() || !storage.archiveCurrent()) return false;
  }
  if (!storage.resetProgress() || !storage.setPending(false)) return false;
  return storage.createCurrent();
}
} // namespace PowerLog

#pragma once
#include "../LogRotation.h"

namespace LogRotationTests {
struct Storage {
  int current = 17, previous = 9;
  bool journal = false;
  uint64_t age = PowerLog::WINDOW_MS;
  int operation = 0, failAt = -1;
  constexpr bool succeeds() { return operation++ != failAt; }
  constexpr bool pending() const { return journal; }
  constexpr bool currentExists() const { return current >= 0; }
  constexpr bool setPending(bool value) { if (!succeeds()) return false; journal = value; return true; }
  constexpr bool removePrevious() { if (!succeeds()) return false; previous = -1; return true; }
  constexpr bool archiveCurrent() { if (!succeeds()) return false; previous = current; current = -1; return true; }
  constexpr bool resetProgress() { if (!succeeds()) return false; age = 0; return true; }
  constexpr bool createCurrent() { if (!succeeds()) return false; if (current < 0) current = 0; return true; }
};

constexpr bool clockChecks() {
  PowerLog::RotationClock clock;
  clock.restore(PowerLog::WINDOW_MS - 1000, 0xffffff00U);
  clock.tick(0x2e7U); // 999 ms across millis() wrap.
  if (clock.due(true)) return false;
  clock.tick(0x2e8U);
  if (!clock.due(true) || clock.due(false)) return false;
  clock.restore(clock.elapsed(), 100);
  clock.tick(1100);
  return clock.elapsed() == PowerLog::WINDOW_MS + 1000 && clock.due(true);
}

constexpr bool recoveryChecks() {
  for (int fault = 0; fault < 6; ++fault) {
    Storage storage;
    storage.failAt = fault;
    if (PowerLog::rotate(storage)) return false;
    // Reboot: only directory contents and persisted metadata survive.
    storage.failAt = -1;
    if (storage.pending() || storage.age >= PowerLog::WINDOW_MS) {
      if (!PowerLog::rotate(storage)) return false;
    } else if (!storage.createCurrent()) return false;
    if (storage.current != 0 || storage.previous != 17 || storage.journal || storage.age != 0) return false;
  }
  Storage ordinary;
  return PowerLog::rotate(ordinary) && ordinary.previous == 17 && ordinary.current == 0;
}
static_assert(clockChecks(), "48-hour boundary, unlimited mode, restart and millis wrap");
static_assert(recoveryChecks(), "Preserve recording across every interrupted rotation operation");
} // namespace LogRotationTests

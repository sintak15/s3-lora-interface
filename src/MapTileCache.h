#pragma once
#include <stddef.h>
#include <stdint.h>

namespace MapTiles {
struct Key {
  int zoom = 0;
  int32_t x = 0;
  int32_t y = 0;
  constexpr bool operator==(const Key& other) const {
    return zoom == other.zoom && x == other.x && y == other.y;
  }
};

// Single-worker index; storage/card generation and root changes clear its context.
// Recency ranks are bounded, avoiding timer/counter wraparound in replacement.
template <size_t Capacity>
class LruIndex {
public:
  static constexpr size_t missing = Capacity;
  constexpr size_t find(Key key, bool update = true) {
    for (size_t i = 0; i < Capacity; ++i) {
      if (entries_[i].valid && entries_[i].key == key) {
        if (update) touch(i);
        return i;
      }
    }
    return missing;
  }
  constexpr size_t replacement() const {
    size_t oldest = 0;
    for (size_t i = 0; i < Capacity; ++i) {
      if (!entries_[i].valid) return i;
      if (entries_[i].age > entries_[oldest].age) oldest = i;
    }
    return oldest;
  }
  constexpr bool valid(size_t slot) const { return slot < Capacity && entries_[slot].valid; }
  constexpr void invalidate(size_t slot) { if (slot < Capacity) entries_[slot].valid = false; }
  constexpr void commit(size_t slot, Key key) {
    if (slot >= Capacity) return;
    entries_[slot] = Entry{key, Capacity, true};
    touch(slot);
  }
  constexpr void clear() { for (auto& entry : entries_) entry = Entry{}; }
  constexpr size_t size() const {
    size_t count = 0;
    for (const auto& entry : entries_) if (entry.valid) ++count;
    return count;
  }
private:
  struct Entry { Key key{}; size_t age = 0; bool valid = false; };
  Entry entries_[Capacity] = {};
  constexpr void touch(size_t slot) {
    const size_t oldAge = entries_[slot].age;
    for (size_t i = 0; i < Capacity; ++i) {
      if (i != slot && entries_[i].valid && entries_[i].age < oldAge) ++entries_[i].age;
    }
    entries_[slot].age = 0;
  }
};
} // namespace MapTiles

#pragma once
#include "../MapTileCache.h"

namespace MapTileCacheTests {
constexpr bool replacementAndHits() {
  MapTiles::LruIndex<3> index;
  const MapTiles::Key a{12, 1, 2}, b{12, 1, 3}, c{12, 2, 3}, d{13, 1, 2};
  if (index.find(a) != index.missing) return false;
  index.commit(0, a); index.commit(1, b); index.commit(2, c);
  if (index.size() != 3 || index.replacement() != 0) return false;
  if (index.find(a) != 0 || index.replacement() != 1) return false;
  index.invalidate(1); // A failed/short read never commits its replacement.
  if (index.find(b) != index.missing || index.size() != 2) return false;
  index.commit(1, d);
  if (index.find(a, false) != 0 || index.find(d, false) != 1 || index.replacement() != 2) return false;
  index.clear(); // A changed root or remounted card invalidates every key.
  return index.size() == 0 && index.find(a) == index.missing;
}
constexpr bool sustainedRecency() {
  MapTiles::LruIndex<2> index;
  index.commit(0, {14, 5, 6}); index.commit(1, {14, 6, 6});
  for (unsigned i = 0; i < 1024; ++i) index.find({14, 5, 6});
  return index.replacement() == 1 && index.find({13, 5, 6}) == index.missing &&
         index.find({14, 5, 7}) == index.missing;
}
static_assert(replacementAndHits(), "Tile LRU must preserve keys, evict oldest and reject uncommitted data");
static_assert(sustainedRecency(), "Repeated hits must not wrap recency or mix tile coordinates/zooms");
} // namespace MapTileCacheTests

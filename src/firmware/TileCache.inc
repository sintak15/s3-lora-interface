// Called only by the single map worker (or its benchmark), with SdLock held.
static void prepareMapTileCache(const MapRenderJob& job) {
  if (mapTileCacheContext != job.storageGeneration || strcmp(mapTileCacheRoot, job.tileRoot) != 0) {
    mapTileIndex.clear();
    mapTileCacheResident.store(0);
    mapTileCacheContext = job.storageGeneration;
    strlcpy(mapTileCacheRoot, job.tileRoot, sizeof(mapTileCacheRoot));
  }
}

static size_t readMapTileBytes(File& file, uint8_t* output, size_t bytes, MapRenderResult& result) {
  uint32_t started = micros();
  size_t read = file.read(output, bytes);
  result.tileReadUs += (uint32_t)(micros() - started);
  mapTileSdReadCalls.fetch_add(1);
  mapTileSdReadBytes.fetch_add((uint32_t)read);
  return read;
}

static const uint16_t* loadCachedMapTile(const MapRenderJob& job, long x, long y, MapRenderResult& result) {
  MapTiles::Key key{job.zoom, (int32_t)x, (int32_t)y};
  size_t slot = mapTileIndex.find(key);
  if (slot != mapTileIndex.missing) {
    mapTileCacheHits.fetch_add(1);
    return mapTileCacheBuffer + slot * (MAP_TILE_BYTES / sizeof(uint16_t));
  }
  mapTileCacheMisses.fetch_add(1);
  char path[96];
  buildTilePath(job.tileRoot, job.zoom, x, y, path, sizeof(path));
  File tile = SD_MMC.open(path, FILE_READ);
  if (!tile) return nullptr; // Missing neighbours are normal at coverage edges.
  if (tile.isDirectory() || tile.size() != MAP_TILE_BYTES) {
    tile.close();
    mapTileCacheReadErrors.fetch_add(1);
    return nullptr;
  }
  slot = mapTileIndex.replacement();
  if (mapTileIndex.valid(slot)) mapTileCacheEvictions.fetch_add(1);
  mapTileIndex.invalidate(slot);
  mapTileCacheResident.store((uint32_t)mapTileIndex.size());
  uint8_t* destination = (uint8_t*)mapTileCacheBuffer + slot * MAP_TILE_BYTES;
  bool complete = true;
  for (size_t offset = 0; offset < MAP_TILE_BYTES; offset += 4096) {
    if (readMapTileBytes(tile, destination + offset, 4096, result) != 4096) { complete = false; break; }
    delay(0);
  }
  tile.close();
  if (!complete) {
    mapTileCacheReadErrors.fetch_add(1);
    return nullptr;
  }
  if (job.storageGeneration != mapStorageGeneration.load()) return nullptr;
  // Never publish a partly loaded tile.
  mapTileIndex.commit(slot, key);
  mapTileCacheLoads.fetch_add(1);
  mapTileCacheResident.store((uint32_t)mapTileIndex.size());
  return (const uint16_t*)destination;
}

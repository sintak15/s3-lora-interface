// Availability is checked against real files for each visible footprint. Planned
// map-pack metadata is deliberately not used as evidence that tiles are installed.
static bool mapCoverageMatches(double lat, double lon, uint16_t height) {
  if (!mapCoverage.ready || mapCoverage.generation != mapStorageGeneration.load() || (uint32_t)(millis() - mapCoverage.checkedMs) >= 10000) return false;
  for (int z = MAP_TILE_MIN_ZOOM; z <= MAP_TILE_MAX_ZOOM; ++z) {
    auto area = MapCoverage::footprint(lonToGlobalPixelX(lon, z), latToGlobalPixelY(lat, z), z, MAP_PLOT_W, height);
    if (!MapCoverage::same(area, mapCoverage.zooms[z - MAP_TILE_MIN_ZOOM].area)) return false;
  }
  return true;
}

static MapCoverage::Profile probeMapCoverage(const MapRenderJob& job) {
  MapCoverage::Profile profile;
  profile.generation = job.storageGeneration;
  SdLock lock(10000);
  if (!lock || !sdStorage.available || job.storageGeneration != mapStorageGeneration.load()) return profile;
  for (int z = MAP_TILE_MIN_ZOOM; z <= MAP_TILE_MAX_ZOOM; ++z) {
    auto& level = profile.zooms[z - MAP_TILE_MIN_ZOOM];
    level.area = MapCoverage::footprint(lonToGlobalPixelX(job.lon, z), latToGlobalPixelY(job.lat, z), z, MAP_PLOT_W, job.plotH);
    level.checked = true;
    for (unsigned i = 0; i < level.area.count; ++i) {
      char path[112];
      buildTilePath(job.tileRoot, z, level.area.cells[i].x, level.area.cells[i].y, path, sizeof(path));
      File tile = SD_MMC.open(path, FILE_READ);
      if (tile && !tile.isDirectory() && tile.size() == MAP_TILE_BYTES) level.validMask |= 1U << i;
      else if (tile || SD_MMC.exists(path)) level.invalidMask |= 1U << i;
      if (tile) tile.close();
      delay(0);
    }
  }
  profile.checkedMs = millis();
  profile.ready = true;
  return profile;
}

static void renderCoveredMap(MapRenderJob job, MapRenderResult& result) {
  result.coverage = probeMapCoverage(job);
  result.zoom = -1;
  if (!result.coverage.ready) { strlcpy(result.status, "coverage unavailable", sizeof(result.status)); return; }
  // Prefer a full viewport. If every complete candidate fails, retain the best
  // readable partial viewport with a valid center rather than hiding all detail.
  for (unsigned pass = 0; pass < 2; ++pass) {
    for (int z = job.zoom; z >= MAP_TILE_MIN_ZOOM; --z) {
      auto& level = result.coverage.zooms[z - MAP_TILE_MIN_ZOOM];
      if (pass == 0 ? !level.complete() : !level.center()) continue;
      MapRenderJob attempt = job;
      attempt.zoom = z;
      double px = lonToGlobalPixelX(job.lon, z), py = latToGlobalPixelY(job.lat, z);
      attempt.centerPixelX = (int)round(px); attempt.centerPixelY = (int)round(py);
      attempt.centerTileX = (long)floor(px / MAP_TILE_SIZE); attempt.centerTileY = (long)floor(py / MAP_TILE_SIZE);
      result.zoom = z;
      result.centerPixelX = attempt.centerPixelX; result.centerPixelY = attempt.centerPixelY;
      result.centerTileX = attempt.centerTileX; result.centerTileY = attempt.centerTileY;
      bool ok = assembleMapTilesToBuffer(attempt, result);
      level.invalidMask |= level.validMask & ~result.drawnMask;
      level.validMask &= result.drawnMask;
      level.readError = !ok || !result.centerTileFound;
      if (ok && (pass == 1 || level.complete())) {
        strlcpy(result.status, level.complete() ? (z == job.zoom ? "drawn" : "coarser map") : "partial map", sizeof(result.status));
        return;
      }
    }
  }
  result.zoom = -1;
  result.drewTiles = result.centerTileFound = false;
  strlcpy(result.status, "no usable coverage", sizeof(result.status));
}

// All filesystem operations below run under the shared SD lock.
struct PowerLogStorage {
  bool pending() const { return batteryCycle.rotationPending; }
  bool setPending(bool value) {
    if (prefs.putBool("batLogPending", value) != sizeof(bool)) return false;
    batteryCycle.rotationPending = value;
    return true;
  }
  bool currentExists() const { return SD_MMC.exists(SD_POWER_LOG_CURRENT_DIR); }
  bool removePrevious() {
    if (!SD_MMC.exists(SD_POWER_LOG_PREVIOUS_DIR)) return true;
    // Only these two owned files may be removed. Unexpected files make rmdir
    // fail safely; never recursively remove user storage.
    for (auto path : {SD_POWER_HEALTH_PREVIOUS_PATH, SD_BATTERY_PREVIOUS_PATH}) {
      if (SD_MMC.exists(path) && !SD_MMC.remove(path)) return false;
    }
    return SD_MMC.rmdir(SD_POWER_LOG_PREVIOUS_DIR);
  }
  bool archiveCurrent() {
    return SD_MMC.rename(SD_POWER_LOG_CURRENT_DIR, SD_POWER_LOG_PREVIOUS_DIR);
  }
  bool resetProgress() {
    if (prefs.putULong64("batLogAge", 0) != sizeof(uint64_t)) return false;
    batteryCycle.rotationClock.restore(0, millis());
    batteryCycle.restoredAgeMs = 0;
    batteryCycle.lastCheckpointMs = millis();
    return true;
  }
  bool createCurrent() {
    return SD_MMC.exists(SD_POWER_LOG_CURRENT_DIR) || SD_MMC.mkdir(SD_POWER_LOG_CURRENT_DIR);
  }
};

static bool powerLogDownloadActive() {
  return sdDownloadPathActive(SD_POWER_HEALTH_PATH) || sdDownloadPathActive(SD_BATTERY_CYCLE_PATH) ||
         sdDownloadPathActive(SD_POWER_HEALTH_PREVIOUS_PATH) || sdDownloadPathActive(SD_BATTERY_PREVIOUS_PATH) ||
         usbSdExportActive();
}

static bool preparePowerLogStorage() {
  PowerLogStorage storage;
  if (!SD_MMC.exists(SD_POWER_LOG_ROOT) && !SD_MMC.mkdir(SD_POWER_LOG_ROOT)) return false;
  if (batteryCycle.rotationPending && !PowerLog::rotate(storage)) return false;
  if (!storage.createCurrent()) return false;
  // Upgrade v0.1.100 in place, retaining its readable and CSV history.
  const char* legacy[] = {"/s3-lora/power-health.log", "/s3-lora/battery-cycle.csv"};
  const char* current[] = {SD_POWER_HEALTH_PATH, SD_BATTERY_CYCLE_PATH};
  for (unsigned i = 0; i < 2; ++i) {
    if (SD_MMC.exists(legacy[i]) && !SD_MMC.exists(current[i]) && !SD_MMC.rename(legacy[i], current[i])) return false;
  }
  if (!batteryCycle.progressRecovered) {
    // The already-flushed readable record provides a 10-second checkpoint,
    // including for repeated short power cycles between NVS checkpoints.
    char tail[3072];
    loadSdTextTail(SD_POWER_HEALTH_PATH, tail, sizeof(tail));
    uint64_t savedAge = batteryCycle.restoredAgeMs;
    const char* at = tail;
    while ((at = strstr(at, "Current log age: "))) {
      at += strlen("Current log age: ");
      char* end = nullptr;
      uint64_t seconds = strtoull(at, &end, 10);
      if (end != at && strncmp(end, " s of powered-on recording", 25) == 0 && seconds <= UINT64_MAX / 1000) {
        savedAge = max(savedAge, seconds * 1000);
      }
    }
    uint64_t elapsedThisBoot = batteryCycle.rotationClock.elapsed() >= batteryCycle.restoredAgeMs ?
                               batteryCycle.rotationClock.elapsed() - batteryCycle.restoredAgeMs : 0;
    batteryCycle.rotationClock.restore(savedAge + elapsedThisBoot, millis());
    batteryCycle.progressRecovered = true;
  }
  batteryCycle.previousAvailable = SD_MMC.exists(SD_POWER_HEALTH_PREVIOUS_PATH);
  batteryCycle.pathsReady = true;
  return true;
}

static bool setPowerLogRotation(bool enabled) {
  if (enabled == batteryCycle.rotationEnabled) return true;
  if (prefs.putBool("batLogLimit", enabled) != sizeof(bool)) return false;
  batteryCycle.rotationEnabled = enabled;
  return true;
}

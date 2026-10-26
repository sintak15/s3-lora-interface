// Mount a validated GPT FAT32 partition as its own bounded block device.
// Sector zero of that device is the existing FAT32 boot sector. GPT metadata
// is outside the device's address range and cannot be written by its callbacks.
#include "../SdGpt.h"
#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>
#include <diskio_impl.h>
#include <esp_vfs_fat.h>
#include <vfs_api.h>

namespace GptStorage {
class Filesystem : public fs::FS {
 public:
  Filesystem() : FS(fs::FSImplPtr(new VFSImpl())) {}
  void path(const char* value) { _impl->mountpoint(value); }
};
static Filesystem files;
static sdmmc_card_t card;
static bool hostReady = false, cardReady = false, vfsReady = false, fatRegistered = false, mounted = false;
static BYTE drive = FF_DRV_NOT_USED;
static char driveName[4] = "";
static uint32_t first = 0, sectors = 0;
static uint32_t headerCrc = 0, tableCrc = 0;
static uint32_t metadataCrc = 0, tableBytes = 0;
static uint64_t primaryTable = 0, backupHeader = 0, backupTable = 0;
static esp_err_t checksumMetadata(uint32_t& checksum) {
  auto* buffer = (uint8_t*)heap_caps_malloc(512, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buffer) return ESP_ERR_NO_MEM;
  struct Span { uint64_t start; uint32_t bytes; };
  const Span spans[] = {{0,512},{1,512},{primaryTable,tableBytes},{backupHeader,512},{backupTable,tableBytes}};
  esp_err_t result = ESP_OK; uint32_t crc = 0xffffffffU;
  for (const auto& span : spans) {
    for (uint32_t offset = 0; offset < span.bytes && result == ESP_OK; offset += 512) {
      result = sdmmc_read_sectors(&card, buffer, span.start + offset / 512, 1);
      if (result == ESP_OK) crc = SdGpt::crcUpdate(crc, buffer, min(uint32_t(512), span.bytes - offset));
    }
    if (result != ESP_OK) break;
  }
  heap_caps_free(buffer); checksum = crc ^ 0xffffffffU; return result;
}
static DSTATUS status(BYTE pdrv) { return cardReady && pdrv == drive ? 0 : STA_NOINIT; }
static DRESULT read(BYTE pdrv, BYTE* data, uint32_t sector, UINT count) {
  if (status(pdrv)) return RES_NOTRDY;
  if (!SdGpt::inside(sector, count, sectors)) return RES_PARERR;
  return sdmmc_read_sectors(&card, data, uint64_t(first) + sector, count) == ESP_OK ? RES_OK : RES_ERROR;
}
static DRESULT write(BYTE pdrv, const BYTE* data, uint32_t sector, UINT count) {
  if (status(pdrv)) return RES_NOTRDY;
  if (!SdGpt::inside(sector, count, sectors)) return RES_PARERR;
  return sdmmc_write_sectors(&card, data, uint64_t(first) + sector, count) == ESP_OK ? RES_OK : RES_ERROR;
}
static DRESULT ioctl(BYTE pdrv, BYTE command, void* data) {
  if (status(pdrv)) return RES_NOTRDY;
  switch (command) {
    case CTRL_SYNC: return sdmmc_get_status(&card) == ESP_OK ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT: if (!data) return RES_PARERR; *(LBA_t*)data = sectors; return RES_OK;
    case GET_SECTOR_SIZE: if (!data) return RES_PARERR; *(WORD*)data = 512; return RES_OK;
    case GET_BLOCK_SIZE: if (!data) return RES_PARERR; *(DWORD*)data = 1; return RES_OK;
    default: return RES_PARERR;
  }
}
static void end() {
  files.path(nullptr);
  if (fatRegistered) f_mount(nullptr, driveName, 0);
  mounted = fatRegistered = false;
  if (vfsReady) esp_vfs_fat_unregister_path("/sdcard");
  vfsReady = false;
  if (drive != FF_DRV_NOT_USED) ff_diskio_unregister(drive);
  drive = FF_DRV_NOT_USED; cardReady = false;
  if (hostReady) sdmmc_host_deinit();
  hostReady = false; first = sectors = 0;
}
static bool begin(bool oneBit, int frequency, char* reason, size_t reasonSize) {
  end();
  headerCrc = tableCrc = metadataCrc = tableBytes = 0;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.flags = oneBit ? SDMMC_HOST_FLAG_1BIT : SDMMC_HOST_FLAG_4BIT;
  host.slot = SDMMC_HOST_SLOT_1; host.max_freq_khz = frequency;
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.clk = (gpio_num_t)SD_CLK_PIN; slot.cmd = (gpio_num_t)SD_CMD_PIN; slot.d0 = (gpio_num_t)SD_D0_PIN;
  slot.d1 = (gpio_num_t)SD_D1_PIN; slot.d2 = (gpio_num_t)SD_D2_PIN; slot.d3 = (gpio_num_t)SD_D3_PIN; slot.width = oneBit ? 1 : 4;
  const char* stage = "host init"; esp_err_t result = sdmmc_host_init(); hostReady = result == ESP_OK;
  if (result == ESP_OK) { stage = "slot init"; result = sdmmc_host_init_slot(host.slot, &slot); }
  if (result == ESP_OK) { stage = "card init"; result = sdmmc_card_init(&host, &card); }
  if (result == ESP_OK && card.csd.sector_size != 512) result = ESP_ERR_NOT_SUPPORTED;
  auto* buffer = result == ESP_OK ? (uint8_t*)heap_caps_malloc(1024, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL) : nullptr;
  if (result == ESP_OK && !buffer) result = ESP_ERR_NO_MEM;
  SdGpt::Header header{};
  if (result == ESP_OK) { stage = "GPT header read"; result = sdmmc_read_sectors(&card, buffer, 1, 1); }
  if (result == ESP_OK) {
    stage = "GPT header checksum/bounds";
    if (!SdGpt::header(buffer, card.csd.capacity, header)) result = ESP_ERR_INVALID_CRC;
    else { headerCrc = SdGpt::le32(buffer + 16); primaryTable = header.table; backupHeader = SdGpt::le64(buffer + 32); tableBytes = header.count * header.entrySize; }
  }
  uint32_t crc = 0xffffffffU, candidates = 0, selectedIndex = UINT32_MAX;
  if (result == ESP_OK) {
    uint32_t bytes = header.count * header.entrySize;
    for (uint32_t offset = 0; offset < bytes && result == ESP_OK; offset += 512) {
      stage = "GPT table read"; result = sdmmc_read_sectors(&card, buffer, header.table + offset / 512, 1);
      if (result != ESP_OK) break;
      uint32_t take = min(uint32_t(512), bytes - offset); crc = SdGpt::crcUpdate(crc, buffer, take);
      for (uint32_t at = 0; at + header.entrySize <= take; at += header.entrySize) {
        uint32_t start, length;
        if (!SdGpt::partition(buffer + at, header, start, length)) continue;
        stage = "FAT32 boot read"; result = sdmmc_read_sectors(&card, buffer + 512, start, 1);
        if (result != ESP_OK) break;
        if (SdGpt::fat32(buffer + 512, length)) { ++candidates; first = start; sectors = length; selectedIndex = (offset + at) / header.entrySize; }
      }
    }
    if (result == ESP_OK) {
      stage = "GPT table checksum"; tableCrc = crc ^ 0xffffffffU;
      if (tableCrc != header.tableCrc) result = ESP_ERR_INVALID_CRC;
      else if (candidates != 1) { stage = "GPT needs exactly one writable Basic Data FAT32 partition"; result = ESP_ERR_NOT_SUPPORTED; }
    }
  }
  if (result == ESP_OK) {
    // Reject overlapping/malformed GPT entries before exposing any writable IO.
    const uint8_t emptyGuid[16] = {};
    for (uint32_t offset = 0; offset < tableBytes && result == ESP_OK; offset += 512) {
      stage = "GPT overlap check"; result = sdmmc_read_sectors(&card, buffer, primaryTable + offset / 512, 1);
      if (result != ESP_OK) break;
      for (uint32_t at = 0; at + header.entrySize <= min(uint32_t(512), tableBytes - offset); at += header.entrySize) {
        const uint8_t* entry = buffer + at;
        if (!memcmp(entry, emptyGuid, 16)) continue;
        uint64_t low = SdGpt::le64(entry + 32), high = SdGpt::le64(entry + 40);
        if (low < header.first || high > header.last || low > high ||
            ((offset + at) / header.entrySize != selectedIndex && low < uint64_t(first) + sectors && high >= first)) { result = ESP_ERR_INVALID_ARG; break; }
      }
    }
  }
  if (result == ESP_OK) {
    stage = "GPT backup bounds"; result = sdmmc_read_sectors(&card, buffer, backupHeader, 1);
    if (result == ESP_OK) {
      backupTable = SdGpt::le64(buffer + 72);
      if (memcmp(buffer, "EFI PART", 8) || SdGpt::le64(buffer + 24) != backupHeader || SdGpt::le64(buffer + 32) != 1 ||
          SdGpt::le32(buffer + 80) != header.count || SdGpt::le32(buffer + 84) != header.entrySize ||
          backupTable <= header.last || backupTable >= backupHeader || (tableBytes + 511) / 512 > backupHeader - backupTable) result = ESP_ERR_INVALID_ARG;
    }
  }
  if (buffer) heap_caps_free(buffer);
  if (result == ESP_OK) { stage = "GPT metadata fingerprint"; result = checksumMetadata(metadataCrc); }
  if (result == ESP_OK) { stage = "FatFs drive allocation"; result = ff_diskio_get_drive(&drive); }
  if (result == ESP_OK) {
    cardReady = true;
    const ff_diskio_impl_t driver = {status, status, read, write, ioctl}; ff_diskio_register(drive, &driver);
    snprintf(driveName, sizeof(driveName), "%u:", unsigned(drive));
    esp_vfs_fat_conf_t config = {"/sdcard", driveName, 8}; FATFS* fatfs = nullptr;
    stage = "VFS register"; result = esp_vfs_fat_register_cfg(&config, &fatfs); vfsReady = result == ESP_OK;
    if (result == ESP_OK) {
      stage = "FAT32 mount"; fatRegistered = true; FRESULT fsResult = f_mount(fatfs, driveName, 1);
      if (fsResult != FR_OK) { snprintf(reason, reasonSize, "GPT FAT32 mount failed: FatFs %u", unsigned(fsResult)); end(); return false; }
      mounted = true; files.path("/sdcard");
      snprintf(reason, reasonSize, "GPT FAT32 mounted: LBA %lu, %lu sectors", (unsigned long)first, (unsigned long)sectors);
      return true;
    }
  }
  snprintf(reason, reasonSize, "%s: %s", stage, esp_err_to_name(result)); end(); return false;
}
static uint64_t totalBytes() {
  FATFS* info = nullptr; DWORD freeClusters;
  return mounted && f_getfree(driveName, &freeClusters, &info) == FR_OK ? uint64_t(info->csize) * (info->n_fatent - 2) * 512 : uint64_t(sectors) * 512;
}
static uint64_t usedBytes() {
  FATFS* info = nullptr; DWORD freeClusters;
  return mounted && f_getfree(driveName, &freeClusters, &info) == FR_OK ? uint64_t(info->csize) * (info->n_fatent - 2 - freeClusters) * 512 : 0;
}
}
static fs::FS& sdCardFilesystem() { return GptStorage::mounted ? static_cast<fs::FS&>(GptStorage::files) : static_cast<fs::FS&>(SD_MMC); }

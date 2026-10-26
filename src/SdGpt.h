#ifndef CYD_SD_GPT_H
#define CYD_SD_GPT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace SdGpt {
constexpr uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
constexpr uint64_t le64(const uint8_t* p) { return uint64_t(le32(p)) | (uint64_t(le32(p + 4)) << 32); }
constexpr uint32_t crcUpdate(uint32_t crc, const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; ++i) { crc ^= data[i]; for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0); }
  return crc;
}
constexpr bool inside(uint32_t sector, uint32_t count, uint32_t length) {
  return count && sector < length && count <= length - sector;
}
constexpr bool same(const uint8_t* bytes, const char* text, size_t size) { for (size_t i = 0; i < size; ++i) if (bytes[i] != uint8_t(text[i])) return false; return true; }
struct Header { uint64_t first, last, table; uint32_t count, entrySize, tableCrc; };
constexpr bool header(const uint8_t* b, uint64_t capacity, Header& out) {
  if (!same(b, "EFI PART", 8) || le32(b + 8) != 0x10000 || le32(b + 12) < 92 || le32(b + 12) > 512 || le64(b + 24) != 1) return false;
  uint32_t crc = 0xffffffffU;
  for (unsigned i = 0; i < le32(b + 12); ++i) { uint8_t byte = i >= 16 && i < 20 ? 0 : b[i]; crc = crcUpdate(crc, &byte, 1); }
  if ((crc ^ 0xffffffffU) != le32(b + 16)) return false;
  out = {le64(b + 40), le64(b + 48), le64(b + 72), le32(b + 80), le32(b + 84), le32(b + 88)};
  uint64_t bytes = uint64_t(out.count) * out.entrySize;
  return capacity > 68 && out.first >= 34 && out.first <= out.last && out.last < capacity - 1 &&
         le64(b + 32) > out.last && le64(b + 32) < capacity && out.table >= 2 &&
         out.count && bytes <= 128 * 1024 && (out.entrySize == 128 || out.entrySize == 256 || out.entrySize == 512) &&
         out.table < out.first && (bytes + 511) / 512 <= out.first - out.table;
}
constexpr bool basicData(const uint8_t* entry) {
  const uint8_t guid[] = {0xa2,0xa0,0xd0,0xeb,0xe5,0xb9,0x33,0x44,0x87,0xc0,0x68,0xb6,0xb7,0x26,0x99,0xc7};
  for (size_t i = 0; i < sizeof(guid); ++i) if (entry[i] != guid[i]) return false;
  return true;
}
constexpr bool partition(const uint8_t* entry, const Header& h, uint32_t& start, uint32_t& length) {
  uint64_t first = le64(entry + 32), last = le64(entry + 40);
  if (!basicData(entry) || (le64(entry + 48) & (uint64_t(1) << 60)) || first < h.first || last > h.last || first > last || last >= UINT32_MAX) return false;
  start = uint32_t(first); length = uint32_t(last - first + 1); return true;
}
constexpr bool fat32(const uint8_t* b, uint32_t partitionLength) {
  uint32_t reserved = b[14] | (uint32_t(b[15]) << 8), total = le32(b + 32), fat = le32(b + 36), root = le32(b + 44);
  uint32_t cluster = b[13], copies = b[16]; uint64_t overhead = reserved + uint64_t(copies) * fat;
  if (b[510] != 0x55 || b[511] != 0xaa || b[11] != 0 || b[12] != 2 || !cluster || cluster > 128 || (cluster & (cluster - 1)) ||
      !reserved || (copies != 1 && copies != 2) || b[17] || b[18] || b[19] || b[20] || b[22] || b[23] || b[42] || b[43] ||
      !fat || !total || total > partitionLength || overhead >= total || !same(b + 82, "FAT32   ", 8)) return false;
  uint64_t clusters = (total - overhead) / cluster;
  return clusters >= 65525 && clusters < 0x0ffffff5 && uint64_t(fat) * 128 >= clusters + 2 && root >= 2 && root < clusters + 2;
}
// These bounds guard every physical write, including overflow at the end.
static_assert(inside(0, 1, 100) && inside(99, 1, 100), "partition endpoints");
static_assert(!inside(100, 1, 100) && !inside(99, 2, 100) && !inside(0, 0, 100), "reject outside writes");
static_assert(!inside(UINT32_MAX - 1, 4, UINT32_MAX), "reject overflowing writes");
}
#endif

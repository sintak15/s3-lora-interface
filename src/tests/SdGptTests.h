#ifndef CYD_SD_GPT_TESTS_H
#define CYD_SD_GPT_TESTS_H
#include "../SdGpt.h"
namespace SdGptTests {
constexpr void put32(uint8_t* p, uint32_t value) { for (unsigned i = 0; i < 4; ++i) p[i] = value >> (8 * i); }
constexpr bool checkHeader(unsigned change) {
  uint8_t bytes[512] = {}; const char signature[] = "EFI PART";
  for (unsigned i = 0; i < 8; ++i) bytes[i] = signature[i];
  put32(bytes + 8, 0x10000); put32(bytes + 12, 92); put32(bytes + 24, 1);
  put32(bytes + 32, 2097151); put32(bytes + 40, 34); put32(bytes + 48, 2097118);
  put32(bytes + 72, change == 2 ? 35 : 2); put32(bytes + 80, change == 3 ? UINT32_MAX : 128);
  put32(bytes + 84, 128);
  if (change == 4) put32(bytes + 32, 2097152);
  put32(bytes + 16, SdGpt::crcUpdate(0xffffffffU, bytes, 92) ^ 0xffffffffU);
  if (change == 1) bytes[20] ^= 1;
  SdGpt::Header parsed{}; return SdGpt::header(bytes, 2097152, parsed);
}
constexpr bool checkFat(unsigned change) {
  uint8_t bytes[512] = {}; bytes[12] = 2; bytes[13] = 1; bytes[14] = 32; bytes[16] = 2;
  put32(bytes + 32, 1048576); put32(bytes + 36, 8192); put32(bytes + 44, 2);
  const char label[] = "FAT32   "; for (unsigned i = 0; i < 8; ++i) bytes[82 + i] = label[i];
  bytes[510] = 0x55; bytes[511] = 0xaa;
  if (change == 1) bytes[12] = 4;
  if (change == 2) bytes[13] = 3;
  if (change == 3) put32(bytes + 36, 1);
  if (change == 4) put32(bytes + 44, UINT32_MAX);
  return SdGpt::fat32(bytes, change == 5 ? 1000000 : 1048576);
}
constexpr bool checkPartition(unsigned change) {
  uint8_t bytes[128] = {0xa2,0xa0,0xd0,0xeb,0xe5,0xb9,0x33,0x44,0x87,0xc0,0x68,0xb6,0xb7,0x26,0x99,0xc7};
  put32(bytes + 32, change == 1 ? 33 : 2048); put32(bytes + 40, 1000000);
  if (change == 2) bytes[55] = 0x10;
  if (change == 3) put32(bytes + 44, 1);
  if (change == 4) bytes[0] = 0;
  SdGpt::Header h{34, 2097118, 2, 128, 128, 0}; uint32_t start = 0, size = 0;
  return SdGpt::partition(bytes, h, start, size);
}
static_assert(checkHeader(0), "accept checksum-valid GPT");
static_assert(!checkHeader(1) && !checkHeader(2) && !checkHeader(3) && !checkHeader(4), "reject corrupt/outside/oversized GPT");
static_assert(checkFat(0), "accept FAT32 boot");
static_assert(!checkFat(1) && !checkFat(2) && !checkFat(3) && !checkFat(4) && !checkFat(5), "reject malformed FAT32 geometry");
static_assert(checkPartition(0), "accept bounded writable Basic Data partition");
static_assert(!checkPartition(1) && !checkPartition(2) && !checkPartition(3) && !checkPartition(4), "reject outside/read-only/wrong-type partitions");
static constexpr uint8_t crcFixture[] = {'1','2','3','4','5','6','7','8','9'};
static_assert((SdGpt::crcUpdate(0xffffffffU, crcFixture, 9) ^ 0xffffffffU) == 0xcbf43926U, "IEEE CRC32 known vector");
}
#endif

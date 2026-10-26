#ifndef CYD_ROM_FILES_H
#define CYD_ROM_FILES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace RomFiles {
static constexpr size_t MAX_ROM_BYTES = 4 * 1024 * 1024;
constexpr bool validName(const char* name) {
  size_t n = 0; if (name) while (name[n]) ++n;
  if (n < 4 || n > 64 || name[0] == '.') return false;
  for (size_t i = 0; i < n; ++i) {
    const unsigned char c = name[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.')) return false;
  }
  return name[n-3] == '.' && (name[n-2] == 'g' || name[n-2] == 'G') && (name[n-1] == 'b' || name[n-1] == 'B');
}
constexpr bool validHeader(const uint8_t* bytes, size_t size) {
  if (!bytes || size < 32768 || size > MAX_ROM_BYTES || bytes[0x148] > 7) return false;
  if (size != (size_t(32768) << bytes[0x148]) || bytes[0x143] == 0xc0) return false;
  uint8_t sum = 0;
  for (size_t i = 0x134; i <= 0x14c; ++i) sum = uint8_t(sum - bytes[i] - 1);
  return sum == bytes[0x14d];
}
}

#endif

#pragma once
#include <stddef.h>
#include <stdint.h>

namespace TileUpload {
constexpr size_t tileBytes = 256 * 256 * 2;
constexpr size_t chunkBytes = 3072;
constexpr uint32_t crcUpdate(uint32_t crc, const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return crc;
}
constexpr bool validCoordinates(uint32_t z, uint32_t x, uint32_t y) {
  return z >= 10 && z <= 14 && x < (1U << z) && y < (1U << z);
}
constexpr bool validChunk(size_t received, size_t offset, size_t count) {
  return offset == received && count > 0 && count <= chunkBytes && received <= tileBytes && count <= tileBytes - received;
}
} // namespace TileUpload

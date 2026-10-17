#pragma once
#include "../TileUpload.h"
namespace TileUploadTests {
constexpr uint8_t check[] = {'1','2','3','4','5','6','7','8','9'};
static_assert((TileUpload::crcUpdate(0xffffffffU, check, sizeof(check)) ^ 0xffffffffU) == 0xcbf43926U, "CRC32 must match the standard host implementation");
static_assert(TileUpload::validCoordinates(14, 16383, 16383) && !TileUpload::validCoordinates(14, 16384, 0) && !TileUpload::validCoordinates(9, 0, 0) && !TileUpload::validCoordinates(32, 0, 0), "Only supported, bounded XYZ coordinates may create paths");
static_assert(TileUpload::validChunk(0, 0, 3072) && TileUpload::validChunk(129024, 129024, 2048) && !TileUpload::validChunk(0, 1, 3072) && !TileUpload::validChunk(131072, 131072, 1) && !TileUpload::validChunk(0, 0, 3073) && !TileUpload::validChunk(0, 0, 0), "Chunks must be sequential, bounded and nonempty");
}

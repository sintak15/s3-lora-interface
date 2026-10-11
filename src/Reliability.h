#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Reliability {

// Decimal only: no whitespace, unsigned minus signs, trailing text or overflow.
constexpr bool parseInteger(const char* text, int64_t minimum, int64_t maximum, int64_t& result) {
  if (!text || !*text || minimum > maximum) return false;
  bool negative = *text == '-';
  if (negative && minimum >= 0) return false;
  if (negative) ++text;
  if (!*text) return false;
  uint64_t magnitude = 0;
  const uint64_t limit = negative ? (uint64_t)(-(minimum + 1)) + 1 : (maximum >= 0 ? (uint64_t)maximum : 0);
  for (; *text; ++text) {
    if (*text < '0' || *text > '9') return false;
    unsigned digit = *text - '0';
    if (magnitude > limit / 10 || (magnitude == limit / 10 && digit > limit % 10)) return false;
    magnitude = magnitude * 10 + digit;
  }
  int64_t value = 0;
  // Subtract before casting so INT64_MIN can be parsed without signed overflow.
  if (negative && magnitude != 0) value = -(int64_t)(magnitude - 1) - 1;
  else if (!negative) value = (int64_t)magnitude;
  if (value < minimum || value > maximum) return false;
  result = value;
  return true;
}

template <size_t Capacity>
class FrameParser {
public:
  enum Result { NONE, HEADER, LENGTH_ACCEPTED, COMPLETE, INVALID_LENGTH };
  static constexpr uint32_t TIMEOUT_MS = 250;

  constexpr bool expire(uint32_t now) {
    if (state_ != MAGIC_1 && (uint32_t)(now - lastByteMs_) >= TIMEOUT_MS) {
      state_ = MAGIC_1;
      position_ = 0;
      return true;
    }
    return false;
  }

  constexpr Result feed(uint8_t byte, uint32_t now) {
    expire(now);
    lastByteMs_ = now;
    switch (state_) {
      case MAGIC_1: state_ = byte == 0x94 ? MAGIC_2 : MAGIC_1; break;
      case MAGIC_2:
        state_ = byte == 0xC3 ? LEN_1 : (byte == 0x94 ? MAGIC_2 : MAGIC_1);
        if (byte == 0xC3) return HEADER;
        break;
      case LEN_1: length_ = (uint16_t)byte << 8; state_ = LEN_2; break;
      case LEN_2:
        length_ |= byte;
        position_ = 0;
        if (length_ == 0 || length_ > Capacity) {
          state_ = byte == 0x94 ? MAGIC_2 : MAGIC_1;
          return INVALID_LENGTH;
        }
        state_ = PAYLOAD;
        return LENGTH_ACCEPTED;
      case PAYLOAD:
        payload_[position_++] = byte;
        if (position_ == length_) {
          state_ = MAGIC_1;
          return COMPLETE;
        }
        break;
    }
    return NONE;
  }

  constexpr const uint8_t* payload() const { return payload_; }
  constexpr size_t length() const { return length_; }

private:
  enum State { MAGIC_1, MAGIC_2, LEN_1, LEN_2, PAYLOAD };
  State state_ = MAGIC_1;
  uint8_t payload_[Capacity] = {};
  uint16_t length_ = 0;
  uint16_t position_ = 0;
  uint32_t lastByteMs_ = 0;
};

}  // namespace Reliability

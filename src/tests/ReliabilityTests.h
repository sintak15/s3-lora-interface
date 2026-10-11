#pragma once

#include "../Reliability.h"

namespace ReliabilityTests {

constexpr bool decimalBoundaries() {
  int64_t value = 99;
  if (!Reliability::parseInteger("0", 0, UINT32_MAX, value) || value != 0) return false;
  if (!Reliability::parseInteger("4294967295", 0, UINT32_MAX, value) || value != UINT32_MAX) return false;
  if (!Reliability::parseInteger("-2147483648", INT32_MIN, INT32_MAX, value) || value != INT32_MIN) return false;
  if (!Reliability::parseInteger("2147483647", INT32_MIN, INT32_MAX, value) || value != INT32_MAX) return false;
  if (!Reliability::parseInteger("-1", -1, 32, value) || value != -1) return false;
  if (!Reliability::parseInteger("7", 0, 7, value) || value != 7) return false;
  if (!Reliability::parseInteger("13", 1, 13, value) || value != 13) return false;
  if (!Reliability::parseInteger("-9223372036854775808", INT64_MIN, INT64_MAX, value) || value != INT64_MIN) return false;
  return true;
}

constexpr bool rejectMalformedNumbers() {
  int64_t value = 99;
  const char* invalid[] = {"", " ", " 1", "1 ", "+1", "-1", "-0", "1x", "1.5", "1e2", "4294967296", "9999999999999999999999999"};
  for (const char* input : invalid) {
    if (Reliability::parseInteger(input, 0, UINT32_MAX, value) || value != 99) return false;
  }
  return !Reliability::parseInteger("8", 0, 7, value) &&
         !Reliability::parseInteger("0", 1, 13, value) &&
         !Reliability::parseInteger("-2147483649", INT32_MIN, INT32_MAX, value) &&
         !Reliability::parseInteger("2147483648", INT32_MIN, INT32_MAX, value);
}

using Parser = Reliability::FrameParser<4>;

constexpr bool decodeWithNoiseAndRepeatedMagic() {
  Parser parser;
  const uint8_t bytes[] = {0x00, 0xC3, 0x94, 0x94, 0xC3, 0, 4, 0x94, 0xC3, 0, 0xFF};
  size_t completions = 0;
  for (uint8_t byte : bytes) if (parser.feed(byte, 10) == Parser::COMPLETE) completions++;
  return completions == 1 && parser.length() == 4 && parser.payload()[0] == 0x94 && parser.payload()[3] == 0xFF;
}

constexpr bool rejectBadLengthsAndRecover() {
  Parser parser;
  const uint8_t bytes[] = {0x94,0xC3,0,0, 0x94,0xC3,0,5, 0x94,0xC3,0,1,42};
  size_t errors = 0, completions = 0;
  for (uint8_t byte : bytes) {
    auto result = parser.feed(byte, 10);
    if (result == Parser::INVALID_LENGTH) errors++;
    if (result == Parser::COMPLETE) completions++;
  }
  return errors == 2 && completions == 1 && parser.payload()[0] == 42;
}

constexpr bool timeoutEveryPartialState() {
  const uint8_t truncated[] = {0x94,0xC3,0,4,99};
  for (size_t length = 1; length <= sizeof(truncated); ++length) {
    Parser parser;
    for (size_t i = 0; i < length; ++i) parser.feed(truncated[i], 100);
    if (parser.expire(349) || !parser.expire(350) || parser.expire(351)) return false;
    const uint8_t frame[] = {0x94,0xC3,0,1,42};
    Parser::Result result = Parser::NONE;
    for (uint8_t byte : frame) result = parser.feed(byte, 351);
    if (result != Parser::COMPLETE || parser.payload()[0] != 42) return false;
  }
  return true;
}

constexpr bool timeoutAcrossMillisWrap() {
  Parser parser;
  parser.feed(0x94, UINT32_MAX - 100);
  return !parser.expire(148) && parser.expire(149);
}

constexpr bool feedExpiresWithoutExplicitPoll() {
  Parser parser;
  parser.feed(0x94, 10);
  parser.feed(0xC3, 10);
  const uint8_t frame[] = {0x94,0xC3,0,1,42};
  Parser::Result result = Parser::NONE;
  for (uint8_t byte : frame) result = parser.feed(byte, 260);
  return result == Parser::COMPLETE && parser.payload()[0] == 42;
}

static_assert(decimalBoundaries(), "Decimal boundaries must round trip");
static_assert(rejectMalformedNumbers(), "Invalid settings must fail without changing the output");
static_assert(decodeWithNoiseAndRepeatedMagic(), "Noise and repeated magic must not lose a valid frame");
static_assert(rejectBadLengthsAndRecover(), "Invalid lengths must not overflow or break later frames");
static_assert(timeoutEveryPartialState(), "Every partial frame state must recover after a timeout");
static_assert(timeoutAcrossMillisWrap(), "Frame timeouts must handle millis wraparound");
static_assert(feedExpiresWithoutExplicitPoll(), "New input after a timeout must resynchronize");

}  // namespace ReliabilityTests

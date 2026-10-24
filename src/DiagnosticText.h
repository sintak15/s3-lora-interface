#pragma once
#include <stddef.h>
#include <stdint.h>

namespace DiagnosticText {
static constexpr uint32_t LABEL_COLOR = 0x85BFFF;

constexpr bool letter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
constexpr size_t labelLength(const char* text) {
  if (!text || !letter(*text)) return 0;
  for (size_t i = 1; i <= 40 && text[i]; ++i) {
    const char c = text[i];
    if (c == ':') return text[i + 1] == ' ' || text[i + 1] == '\n' || !text[i + 1] ? i + 1 : 0;
    if (!letter(c) && !(c >= '0' && c <= '9') && c != ' ' && c != '/' && c != '-' && c != '_' && c != '(' && c != ')' && c != '%') return 0;
  }
  return 0;
}

// Add LVGL color commands only around structured labels. Literal # characters
// in SSIDs, node names and values are escaped, so data cannot become markup.
// Every command is written atomically; callers can fall back to plain text if
// a future screen exceeds their buffer, rather than display truncated readings.
constexpr bool format(char* out, size_t capacity, const char* text) {
  if (!out || !capacity) return false;
  out[0] = '\0';
  if (!text) return true;
  size_t used = 0;
  for (size_t i = 0; text[i];) {
    const bool boundary = i == 0 || text[i - 1] == '\n' ||
                          (i >= 2 && text[i - 1] == ' ' && text[i - 2] == ' ');
    const size_t length = boundary ? labelLength(text + i) : 0;
    if (length) {
      if (used + length + 9 >= capacity) return false;
      const char* command = "#85BFFF ";
      for (size_t j = 0; j < 8; ++j) out[used++] = command[j];
      for (size_t j = 0; j < length; ++j) out[used++] = text[i++];
      out[used++] = '#';
    } else {
      const size_t needed = text[i] == '#' ? 2 : 1;
      if (used + needed >= capacity) return false;
      out[used++] = text[i];
      if (text[i] == '#') out[used++] = '#';
      ++i;
    }
    out[used] = '\0';
  }
  return true;
}
} // namespace DiagnosticText

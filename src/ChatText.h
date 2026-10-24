#pragma once
#include <stddef.h>

namespace ChatText {
enum class Direction { Continuation, Mine, Other };

constexpr Direction direction(const char* line) {
  if (!line || *line != '[') return Direction::Continuation;
  size_t i = 1;
  while (line[i] && line[i] != '\n' && line[i] != ']') ++i;
  if (line[i] != ']' || line[i + 1] != ' ' || line[i + 2] != '[') return Direction::Continuation;
  i += 3;
  const size_t sender = i;
  while (line[i] && line[i] != '\n' && line[i] != ']') ++i;
  if (line[i] != ']' || line[i + 1] != ' ') return Direction::Continuation;
  return i - sender == 2 && line[sender] == 'm' && line[sender + 1] == 'e' ? Direction::Mine : Direction::Other;
}

// Escape data before adding LVGL recolor commands. Continuation lines inherit
// their message's direction; literal '#' can never inject a color command.
constexpr bool format(char* out, size_t capacity, const char* input) {
  if (!out || !capacity) return false;
  out[0] = 0;
  if (!input) return true;
  size_t used = 0;
  bool colored = false;
  for (size_t i = 0; input[i]; ++i) {
    if (i == 0 || input[i - 1] == '\n') {
      Direction kind = direction(input + i);
      if (kind != Direction::Continuation) {
        if (used + (colored ? 9 : 8) + 2 >= capacity) return false;
        if (colored) out[used++] = '#';
        const char* color = kind == Direction::Mine ? "#85BFFF " : "#F4F8FF ";
        for (size_t j = 0; j < 8; ++j) out[used++] = color[j];
        colored = true;
      }
    }
    if (used + (input[i] == '#' ? 2 : 1) + (colored ? 1 : 0) >= capacity) return false;
    out[used++] = input[i];
    if (input[i] == '#') out[used++] = '#';
    out[used] = 0;
  }
  if (colored) out[used++] = '#';
  out[used] = 0;
  return true;
}
} // namespace ChatText

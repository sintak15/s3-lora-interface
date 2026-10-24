#pragma once
#include "../ChatText.h"

static_assert(ChatText::direction("[03:00] [me] hi") == ChatText::Direction::Mine, "Own direction marker");
static_assert(ChatText::direction("[03:00] [from me] hi") == ChatText::Direction::Other, "Remote name cannot become own direction");
static_assert(ChatText::direction("[03:00] [someone] hi") == ChatText::Direction::Other, "Legacy incoming history");
static_assert(ChatText::direction("  [03:00] [me] quoted") == ChatText::Direction::Continuation, "Indented message content");
static_assert(ChatText::direction("[") == ChatText::Direction::Continuation, "Truncated history prefix");
constexpr bool chatColorChecks() {
  char out[128] = {};
  if (!ChatText::format(out, sizeof(out), "[03:00] [me] #FF0000 hi")) return false;
  if (out[0] != '#' || out[1] != '8' || out[2] != '5') return false;
  bool escaped = false;
  for (size_t i = 1; out[i]; ++i) if (out[i] == '#' && out[i + 1] == '#') escaped = true;
  char small[12] = {};
  return escaped && !ChatText::format(small, sizeof(small), "[03:00] [me] long text");
}
static_assert(chatColorChecks(), "Direction colors, literal markup escaping and bounded output");

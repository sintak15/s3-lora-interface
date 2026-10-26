#ifndef CYD_CONVERSATION_ROUTING_H
#define CYD_CONVERSATION_ROUTING_H
#include <stdint.h>
namespace ConversationRouting {
constexpr uint32_t peer(uint32_t from, uint32_t to, uint32_t ours) {
  return !ours || !to || to == 0xffffffffU ? 0 : from == ours ? to : to == ours ? from : 0;
}
}

#endif

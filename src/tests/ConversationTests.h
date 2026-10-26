#pragma once
#include "../ConversationRouting.h"
#include "../games/RomFiles.h"
#include "../games/GameBoyInput.h"
static_assert(ConversationRouting::peer(1, 2, 1) == 2, "outgoing thread uses destination");
static_assert(ConversationRouting::peer(2, 1, 1) == 2, "reply stays in same thread");
static_assert(ConversationRouting::peer(3, 1, 1) == 3, "other sender has another thread");
static_assert(ConversationRouting::peer(2, 3, 1) == 0, "overheard third-party DM is excluded");
static_assert(ConversationRouting::peer(2, 0xffffffffU, 1) == 0, "broadcast is not a DM");
static_assert(ConversationRouting::peer(2, 1, 0) == 0, "unknown identity cannot assign a peer");
static_assert(RomFiles::validName("Homebrew 1.gb") && RomFiles::validName("TEST.GB"), "ROM names");
static_assert(!RomFiles::validName("../secret.gb") && !RomFiles::validName("/test.gb") && !RomFiles::validName("test.gbc") && !RomFiles::validName("a\\b.gb"), "reject traversal and unsupported extensions");
static_assert(GameBoyInput::at(50, 170) == GAMEBOY_UP && GameBoyInput::at(200, 180) == GAMEBOY_A, "controller hit regions");
static_assert((GameBoyInput::at(50, 170) | GameBoyInput::at(200, 180)) == (GAMEBOY_UP | GAMEBOY_A), "two touches combine direction and A");
static_assert(GameBoyInput::at(0, 0) == 0 && GameBoyInput::at(240, 180) == 0, "outside controller releases keys");

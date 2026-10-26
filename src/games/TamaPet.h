#ifndef CYD_TAMA_PET_H
#define CYD_TAMA_PET_H
#include <stdint.h>
// Compact CYD care-loop adaptation of CyberXcyborg/ESP32-TamaPetchi.
// Copyright (c) 2025 CyberXcyborg, MIT; see ../vendor/tamapetchi/LICENSE.txt.
namespace TamaGame {
struct Pet {
  uint8_t hunger = 50, happiness = 50, health = 80, energy = 100, cleanliness = 80;
  bool sleeping = false;
  uint32_t age = 0, elapsed = 0;
  static uint8_t clamp(int value) { return value < 0 ? 0 : value > 100 ? 100 : value; }
  void update(uint32_t ms) {
    elapsed += ms;
    while (elapsed >= 60000) {
      elapsed -= 60000; ++age;
      hunger = clamp(hunger - (sleeping ? 2 : 5));
      happiness = clamp(happiness - (sleeping ? 1 : 3));
      energy = clamp(energy + (sleeping ? 10 : -2));
      cleanliness = clamp(cleanliness - (sleeping ? 2 : 4));
      if (hunger < 20 || cleanliness < 20) health = clamp(health - 5);
      if (energy == 100) sleeping = false;
    }
  }
  void action(unsigned id) {
    if (id == 0) { if (hunger < 20) health = clamp(health + 5); hunger = clamp(hunger + 20); energy = clamp(energy - 5); }
    if (id == 1 && energy >= 10) { happiness = clamp(happiness + 15); energy = clamp(energy - 15); hunger = clamp(hunger - 10); }
    if (id == 2) { cleanliness = 100; health = clamp(health + 5); }
    if (id == 3) sleeping = !sleeping;
    if (id == 4) health = 100;
  }
  const char* mood() const { return health == 0 ? "Needs healing" : sleeping ? "Sleeping" : health < 30 ? "Sick" : hunger < 20 ? "Hungry" : happiness < 30 ? "Bored" : "Happy"; }
};
}

#endif

#pragma once
#include <cstdint>
#include <string>

struct RaceStats {
  std::string name;

  float health;
  float mana;
  float recovery;

  uint8_t constitution;
  uint8_t intelligence;
  uint8_t strength;
  uint8_t agility;
};
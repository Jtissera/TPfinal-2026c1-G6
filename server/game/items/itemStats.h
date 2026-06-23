#pragma once
#include <cstdint>
#include <string>

struct ItemStats {
  uint16_t damageMin = 0;
  uint16_t damageMax = 0;
  uint16_t defenseMin = 0;
  uint16_t defenseMax = 0;
  uint16_t healAmount = 0;
  uint16_t manaAmount = 0;
  uint16_t manaCost = 0;
  bool isRanged = false;
  std::string visualEffectId;
};
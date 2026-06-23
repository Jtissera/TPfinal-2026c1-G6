#pragma once
#include <cstdint>

enum class CheatType : uint8_t
{
  INFINITE_HP = 0,
  INFINITE_MANA = 1,
  DIE = 2,
  LEVEL_UP = 3,
  ADD_GOLD = 4,
};
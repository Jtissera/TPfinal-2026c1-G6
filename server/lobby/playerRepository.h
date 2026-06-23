#pragma once

#include <cstdint>
#include <unordered_map>

#include "../game/player/Player.h"

class PlayerRepository
{
public:
  void save(uint32_t clientId, Player player);
  Player *get(uint32_t clientId);
  void remove(uint32_t clientId);

private:
  std::unordered_map<uint32_t, Player> players;
};
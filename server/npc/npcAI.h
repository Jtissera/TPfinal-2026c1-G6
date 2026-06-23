#pragma once
#include "../game/player/Player.h"
#include "npc.h"
#include "npcIntent.h"
#include <cmath>
#include <cstdlib>
#include <toml++/toml.hpp>
#include <unordered_map>
#include <utility>

class NpcAI
{
public:
  explicit NpcAI(const toml::table &config);

  NpcIntent decide(const Npc &npc,
                   const std::unordered_map<uint32_t, Player> &players) const;

private:
  int meleeRange;

  uint32_t findClosestPlayerId(
      const Npc &npc,
      const std::unordered_map<uint32_t, Player> &players) const;

  int distance(int x1, int y1, int x2, int y2) const;
  std::pair<int, int> stepTowards(int fromX, int fromY, int toX, int toY) const;
};
#pragma once
#include "../game/player/Player.h"
#include "npc.h"
#include "npcIntent.h"
#include <unordered_map>

class NpcAI {
public:
  NpcIntent decide(const Npc &npc,
                   const std::unordered_map<uint32_t, Player> &players) const;

private:
  uint32_t findClosestPlayerId(
      const Npc &npc,
      const std::unordered_map<uint32_t, Player> &players) const;
  int distance(int x1, int y1, int x2, int y2) const;
  std::pair<int, int> stepTowards(int fromX, int fromY, int toX, int toY) const;
};
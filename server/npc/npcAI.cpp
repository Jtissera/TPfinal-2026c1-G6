#include "npcAI.h"
#include <cmath>
#include <cstdlib>

static constexpr int MELEE_RANGE = 1; // cambiar

NpcIntent
NpcAI::decide(const Npc &npc,
              const std::unordered_map<uint32_t, Player> &players) const {
  if (!npc.isAlive())
    return {};

  uint32_t targetId = findClosestPlayerId(npc, players);

  if (targetId != 0) {
    const Player &target = players.at(targetId);
    int dx = std::abs(npc.getTileX() - target.getTileX());
    int dy = std::abs(npc.getTileY() - target.getTileY());

    if (dx <= MELEE_RANGE && dy <= MELEE_RANGE) {
      return {NpcIntent::Type::ATTACK, 0, 0, targetId, NpcState::ATTACKING};
    }

    auto [tx, ty] = stepTowards(npc.getTileX(), npc.getTileY(),
                                target.getTileX(), target.getTileY());
    return {NpcIntent::Type::MOVE, tx, ty, targetId, NpcState::CHASING};
  }

  int distHome = distance(npc.getTileX(), npc.getTileY(), npc.getSpawnPixelX(),
                          npc.getSpawnPixelY());

  if (distHome > 0) {
    auto [tx, ty] = stepTowards(npc.getTileX(), npc.getTileY(),
                                npc.getSpawnPixelX(), npc.getSpawnPixelY());
    NpcState next = (distHome == 1) ? NpcState::IDLE : NpcState::RETURNING;
    return {NpcIntent::Type::MOVE, tx, ty, 0, next};
  }

  return {NpcIntent::Type::IDLE, 0, 0, 0, NpcState::IDLE};
}

uint32_t NpcAI::findClosestPlayerId(
    const Npc &npc, const std::unordered_map<uint32_t, Player> &players) const {

  uint32_t closestId = 0;
  int minDist = npc.getDetectionRangePx() + 1;

  for (const auto &[id, player] : players) {
    if (!player.isAlive())
      continue;
    int dist = distance(npc.getTileX(), npc.getTileY(), player.getTileX(),
                        player.getTileY());
    if (dist < minDist) {
      minDist = dist;
      closestId = id;
    }
  }

  return closestId;
}

int NpcAI::distance(int x1, int y1, int x2, int y2) const {
  return std::max(std::abs(x1 - x2), std::abs(y1 - y2));
}

std::pair<int, int> NpcAI::stepTowards(int fromX, int fromY, int toX,
                                       int toY) const {
  int tx = fromX + (toX > fromX ? 1 : toX < fromX ? -1 : 0);
  int ty = fromY + (toY > fromY ? 1 : toY < fromY ? -1 : 0);
  return {tx, ty};
}

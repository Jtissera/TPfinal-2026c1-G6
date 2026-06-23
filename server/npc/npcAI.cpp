#include "npcAI.h"

NpcAI::NpcAI(const toml::table &config)
    : meleeRange(config["npc_ai"]["melee_range"].value_or(1))
{
}

NpcIntent NpcAI::decide(const Npc &npc,
                        const std::unordered_map<uint32_t, Player> &players) const
{
  if (!npc.isAlive())
  {
    return {};
  }

  uint32_t targetId = findClosestPlayerId(npc, players);

  if (targetId != 0)
  {
    const Player &target = players.at(targetId);
    int dx = std::abs(npc.getTileX() - target.getTileX());
    int dy = std::abs(npc.getTileY() - target.getTileY());

    if (dx <= meleeRange && dy <= meleeRange)
    {
      return {NpcIntent::Type::ATTACK, 0, 0, targetId, NpcState::ATTACKING};
    }

    std::pair<int, int> step = stepTowards(
        npc.getTileX(), npc.getTileY(),
        target.getTileX(), target.getTileY());
    return {NpcIntent::Type::MOVE, step.first, step.second, targetId, NpcState::CHASING};
  }

  int distHome = distance(npc.getTileX(), npc.getTileY(),
                          npc.getSpawnTileX(), npc.getSpawnTileY());

  if (distHome > 0)
  {
    std::pair<int, int> step = stepTowards(
        npc.getTileX(), npc.getTileY(),
        npc.getSpawnTileX(), npc.getSpawnTileY());
    NpcState next = (distHome == 1) ? NpcState::IDLE : NpcState::RETURNING;
    return {NpcIntent::Type::MOVE, step.first, step.second, 0, next};
  }

  return {NpcIntent::Type::IDLE, 0, 0, 0, NpcState::IDLE};
}

uint32_t NpcAI::findClosestPlayerId(
    const Npc &npc,
    const std::unordered_map<uint32_t, Player> &players) const
{

  const uint32_t currentTarget = npc.getTargetId();
  if (currentTarget != 0)
  {
    std::unordered_map<uint32_t, Player>::const_iterator it = players.find(currentTarget);
    if (it != players.end() && it->second.isAlive())
    {
      int dist = distance(npc.getTileX(), npc.getTileY(),
                          it->second.getTileX(), it->second.getTileY());
      if (dist <= npc.getDetectionRange())
      {
        return currentTarget;
      }
    }
  }

  uint32_t closestId = 0;
  int minDist = npc.getDetectionRange() + 1;

  for (const std::pair<const uint32_t, Player> &entry : players)
  {
    if (!entry.second.isAlive())
    {
      continue;
    }
    int dist = distance(npc.getTileX(), npc.getTileY(),
                        entry.second.getTileX(), entry.second.getTileY());
    if (dist < minDist)
    {
      minDist = dist;
      closestId = entry.first;
    }
  }

  return closestId;
}

int NpcAI::distance(int x1, int y1, int x2, int y2) const
{
  return std::max(std::abs(x1 - x2), std::abs(y1 - y2));
}

std::pair<int, int> NpcAI::stepTowards(int fromX, int fromY, int toX, int toY) const
{
  int tx = fromX + (toX > fromX ? 1 : toX < fromX ? -1
                                                  : 0);
  int ty = fromY + (toY > fromY ? 1 : toY < fromY ? -1
                                                  : 0);
  return {tx, ty};
}
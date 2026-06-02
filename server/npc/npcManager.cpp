#include "npcManager.h"
#include <cstdlib>

NpcManager::NpcManager(NpcFactory &factory, const CollisionSystem &collision,
                       const MapData &mapData)
    : factory(factory), collision(collision), mapData(mapData) {}

uint32_t NpcManager::spawnNpc(const std::string &typeName, int tileX,
                              int tileY)
{
  Npc npc = factory.create(typeName, tileX, tileY);
  uint32_t id = npc.getId();
  npcs.emplace(id, std::move(npc));
  return id;
}

void NpcManager::applyMove(uint32_t npcId, int toX, int toY)
{
  auto it = npcs.find(npcId);
  if (it != npcs.end())
  {
    it->second.setTilePos(toX, toY);
    it->second.resetMoveCooldown();
  }
}

NpcTickResult
NpcManager::tick(const std::unordered_map<uint32_t, Player> &players)
{
  NpcTickResult result;

  for (auto &[id, npc] : npcs)
  {
    if (!npc.isAlive())
      continue;

    NpcIntent intent = ai.decide(npc, players);

    npc.setState(intent.nextState);
    npc.setTargetId(intent.targetId);

    if (intent.type == NpcIntent::Type::MOVE && npc.canMove())
    {
      result.moveIntents.push_back(
          {id, npc.getTileX(), npc.getTileY(), intent.tileX, intent.tileY});
    }
    else if (intent.type == NpcIntent::Type::ATTACK && npc.canAttack())
    {
      result.attacks.push_back({intent.targetId, rollDamage(npc.getStats()), npc.getStats().xpMultiplier});
      npc.resetAttackCooldown();
    }
  }

  std::vector<uint32_t> toErase;

  for (auto &[id, npc] : npcs)
  {
    if (!npc.isAlive())
    {
      result.deaths.push_back(buildDeathResult(npc));
      toErase.push_back(id);
    }
  }
  for (uint32_t id : toErase)
    npcs.erase(id);

  return result;
}

int16_t NpcManager::rollDamage(const NpcStats &stats) const
{
  int range = stats.damageMax - stats.damageMin;
  return static_cast<int16_t>(stats.damageMin +
                              (range > 0 ? std::rand() % range : 0));
}

NpcDeathResult NpcManager::buildDeathResult(const Npc &npc) const
{
  NpcDeathResult d;
  d.npcId = npc.getId();
  d.tileX = npc.getTileX();
  d.tileY = npc.getTileY();
  d.goldDrop = 0;

  const NpcStats &stats = npc.getStats();
  int roll = std::rand() % 100;

  float effectiveGoldChance = 8.0f * stats.goldMultiplier;
  float effectiveItemChance = 1.0f * stats.itemMultiplier;

  if (roll < static_cast<int>(effectiveGoldChance))
  {
    float factor = 0.01f + (std::rand() % 100) / 100.0f * 0.19f;
    d.goldDrop = static_cast<uint32_t>(
        factor * npc.getMaxHp() * stats.goldMultiplier);
  }
  else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance))
  {
    d.itemDrop = (std::rand() % 2 == 0) ? "pocion_vida" : "pocion_mana";
  }
  else if (roll < static_cast<int>(effectiveGoldChance + effectiveItemChance * 2))
  {
    d.itemDrop = "espada";
  }

  return d;
}

bool NpcManager::isSameZone(int tileX, int tileY, ZoneType zone) const
{
  if (!collision.isInBounds(tileX, tileY))
    return false;
  return mapData.at(tileX, tileY).zone == zone;
}
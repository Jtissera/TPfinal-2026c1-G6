#pragma once
#include "../game/player/Player.h"
#include "../world/CollisionSystem.h"
#include "../world/OccupancySystem.h"
#include "npc.h"
#include "npcAI.h"
#include "npcFactory.h"
#include "npcResult.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <toml++/toml.hpp>
#include <unordered_map>
#include <vector>

class NpcManager
{
public:
  NpcManager(NpcFactory &factory,
             const CollisionSystem &collision,
             const MapData &mapData,
             const toml::table &config);

  uint32_t spawnNpc(const std::string &typeName, int tileX, int tileY);
  void removeNpc(uint32_t npcId);
  void applyMove(uint32_t npcId, int toX, int toY);

  NpcTickResult tick(const std::unordered_map<uint32_t, Player> &players);

  const std::unordered_map<uint32_t, Npc> &getNpcs() const;
  int count() const;
  bool isSameZone(int tileX, int tileY, ZoneType zone) const;
  bool hasNpc(uint32_t npcId) const;

  bool damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId);
  Npc &getNpc(uint32_t npcId);
  const Npc &getNpc(uint32_t npcId) const;
  Npc *findNpc(uint32_t npcId);
  const Npc *findNpc(uint32_t npcId) const;

  void startRespawn(uint32_t npcId, float respawnMs);
  std::vector<uint32_t> tickRespawns(float deltaMs);

  NpcDeathResult buildDeathResult(const Npc &npc, uint32_t killerPlayerId) const;

private:
  NpcFactory &factory;
  const MapData &mapData;
  const CollisionSystem &collision;
  NpcAI ai;
  std::unordered_map<uint32_t, Npc> npcs;

  float baseGoldChance;
  float baseItemChance;
  float goldFactorMin;
  float goldFactorRange;

  int16_t rollDamage(const NpcStats &stats) const;
};
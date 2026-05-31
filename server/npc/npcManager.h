#pragma once
#include "npc.h"
#include "npcAI.h"
#include "npcFactory.h"
#include "../game/Player.h"
#include "../world/CollisionSystem.h"
#include "../world/OccupancySystem.h"
#include "npcResult.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

class NpcManager {
public:
    NpcManager(NpcFactory& factory,
               const CollisionSystem& collision);

    uint32_t spawnNpc(const std::string& typeName, int tileX, int tileY);
    void applyMove(uint32_t npcId, int toX, int toY);
    NpcTickResult tick(const std::unordered_map<uint32_t, Player>& players);

    const std::unordered_map<uint32_t, Npc>& getNpcs() const { return npcs; }
    int count() const { return static_cast<int>(npcs.size()); }

    bool hasNpc(uint32_t npcId) const;

    bool damageNpc(uint32_t npcId,int16_t damage,uint32_t attackerPlayerId);
    Npc& getNpc(uint32_t npcId);
    const Npc& getNpc(uint32_t npcId) const;

private:
    NpcFactory&            factory;
    const CollisionSystem& collision;
    NpcAI                  ai;
    std::unordered_map<uint32_t, Npc> npcs;
 
    int16_t          rollDamage(const NpcStats& stats) const;
    NpcDeathResult   buildDeathResult(const Npc& npc) const;
};
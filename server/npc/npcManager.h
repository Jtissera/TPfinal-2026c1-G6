#pragma once
#include "../game/player/Player.h"
#include "../world/CollisionSystem.h"
#include "npc.h"
#include "npcAI.h"
#include "npcFactory.h"
#include "npcResult.h"
#include <string>
#include <unordered_map>
#include <vector>

class NpcManager {
public:
    NpcManager(NpcFactory& factory,
               const CollisionSystem& collision,
               const MapData& mapData,
               int tileSize);

    // Spawn recibe pixeles (centro del tile donde aparece)
    uint32_t spawnNpc(const std::string& typeName,
                      float pixelX, float pixelY);

    // Aplica movimiento validado por GameWorld
    void applyMove(uint32_t npcId, float toX, float toY);

    NpcTickResult tick(const std::unordered_map<uint32_t, Player>& players);

    const std::unordered_map<uint32_t, Npc>& getNpcs() const { return npcs; }
    int count() const { return static_cast<int>(npcs.size()); }

    bool isSameZone(float pixelX, float pixelY, ZoneType zone) const;

private:
    NpcFactory&           factory;
    const MapData&        mapData;
    const CollisionSystem& collision;
    NpcAI                 ai;
    int                   tileSize;
    std::unordered_map<uint32_t, Npc> npcs;

    int16_t          rollDamage(const NpcStats& stats) const;
    NpcDeathResult   buildDeathResult(const Npc& npc) const;
};
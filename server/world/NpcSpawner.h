#pragma once

#include "../npc/npcManager.h"
#include "OccupancySystem.h"
#include "CollisionSystem.h"
#include "../../editor/map/mapData.h"
#include "WorldTickResult.h"

#include <toml++/toml.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class NpcSpawner
{
public:
    NpcSpawner(NpcManager &npcManager,
               OccupancySystem &occupancy,
               const CollisionSystem &collision,
               const toml::table &config);

    void spawnMapNpcs(const MapData &mapData);
    void spawnNpc(const std::string &typeName, int tileX, int tileY);

    void startNpcRespawn(uint32_t npcId);

    std::vector<uint32_t> tickDynamicSpawns();
    void processRespawns(float deltaMs, WorldTickResult &result);

private:
    struct SpawnPoint
    {
        std::string typeName;
        int x;
        int y;
    };

    void spawnCombatNpcs(const MapData &mapData);
    void spawnCityNpcs(const MapData &mapData);

    std::optional<uint32_t> trySpawnAround(const std::string &typeName, int x, int y);
    bool placeRespawnedNpc(Npc &npc, WorldTickResult &result);
    std::optional<std::pair<int, int>> findAndOccupyAdjacentTile(int tileX, int tileY,
                                                                   uint32_t entityId);

    NpcManager &npcManager;
    OccupancySystem &occupancy;
    const CollisionSystem &collision;

    int tileSize;
    float npcRespawnDelayMs;
    int spawnEveryNTicks;
    int maxNpcs;
    int spawnBatchSize;
    int spawnTickCounter;

    std::vector<SpawnPoint> spawnPoints;
};
#pragma once

#include "../../common/npcType.h"
#include "../../editor/map/mapData.h"
#include "../npc/npcManager.h"
#include "CollisionSystem.h"
#include "OccupancySystem.h"
#include <cstdint>
#include <string>
#include <toml++/toml.hpp>
#include <vector>
#include <optional>

class SpawnManager {
public:
  SpawnManager(const toml::table &config, NpcManager &npcManager,
               const CollisionSystem &collision, OccupancySystem &occupancy);

  std::optional<uint32_t> spawnNpc(const std::string& typeName, int tileX, int tileY);
  std::vector<uint32_t> tick();
  std::optional<uint32_t> trySpawnAround(const std::string& typeName, int x, int y);
  void loadSpawnPoints(const MapData &mapData);

private:
  NpcManager &npcManager;
  const CollisionSystem &collision;
  OccupancySystem &occupancy;

  int spawnTickCounter = 0;
  int spawnEveryNTicks;
  int maxNpcs;
  int spawnBatchSize;

  struct SpawnPoint {
    std::string typeName;
    int x, y;
  };
  std::vector<SpawnPoint> spawnPoints;

};
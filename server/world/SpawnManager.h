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

class SpawnManager {
public:
  SpawnManager(const toml::table &config, NpcManager &npcManager,
               const CollisionSystem &collision, OccupancySystem &occupancy);

  void loadSpawnPoints(const MapData &mapData);
  void spawnNpc(const std::string &typeName, int tileX, int tileY);
  void tick();

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

  void trySpawnAround(const std::string &typeName, int x, int y);
};
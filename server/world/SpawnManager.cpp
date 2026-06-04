#include "SpawnManager.h"
#include <cstdlib>

SpawnManager::SpawnManager(const toml::table &config, NpcManager &npcManager,
                           const CollisionSystem &collision,
                           OccupancySystem &occupancy)
    : npcManager(npcManager), collision(collision), occupancy(occupancy),
      spawnEveryNTicks(config["npc"]["spawn_interval_ticks"].value_or(200)),
      maxNpcs(config["npc"]["max_population"].value_or(20)),
      spawnBatchSize(config["npc"]["spawn_batch_size"].value_or(4)) {}

void SpawnManager::loadSpawnPoints(const MapData &mapData)
{
  for (uint16_t y = 0; y < mapData.height(); y++)
  {
    for (uint16_t x = 0; x < mapData.width(); x++)
    {
      const Tile &tile = mapData.at(x, y);
      if (tile.npc == NpcType::NONE)
        continue;

      if (!isSpawnable(tile.npc))
        continue;
      std::string typeName = npcTypeKey(tile.npc);

      spawnPoints.push_back({typeName, x, y});
      trySpawnAround(typeName, x, y);
    }
  }
}

void SpawnManager::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
  if (!collision.isWalkable(tileX, tileY))
    return;
  if (occupancy.isOccupied(tileX, tileY))
    return;
  uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);
  occupancy.occupy(tileX, tileY, npcId);
}

void SpawnManager::trySpawnAround(const std::string &typeName, int x, int y)
{
  int attempts = 0;
  while (attempts < 10)
  {
    int dx = (std::rand() % 7) - 3;
    int dy = (std::rand() % 7) - 3;
    int tx = x + dx;
    int ty = y + dy;
    if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
    {
      spawnNpc(typeName, tx, ty);
      return;
    }
    attempts++;
  }
}

void SpawnManager::tick()
{
  spawnTickCounter++;
  if (spawnTickCounter < spawnEveryNTicks)
    return;

  spawnTickCounter = 0;
  int toSpawn = std::min(spawnBatchSize, maxNpcs - npcManager.count());

  for (int i = 0; i < toSpawn && !spawnPoints.empty(); i++)
  {
    auto &point = spawnPoints[std::rand() % spawnPoints.size()];
    trySpawnAround(point.typeName, point.x, point.y);
  }
}
#include "SpawnManager.h"

#include <cstdlib>
#include <iostream>

SpawnManager::SpawnManager(
    const toml::table &config,
    NpcManager &npcManager,
    const CollisionSystem &collision)
    : npcManager(npcManager),
      collision(collision),
      tileSize(config["world"]["tile_size"].value_or(96)),
      spawnEveryNTicks(
          config["npc"]["spawn_interval_ticks"].value_or(200)),
      maxNpcs(
          config["npc"]["max_population"].value_or(20)),
      spawnBatchSize(
          config["npc"]["spawn_batch_size"].value_or(4))
{
}

void SpawnManager::loadSpawnPoints(
    const MapData &mapData)
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

      std::string typeName =
          npcTypeKey(tile.npc);

      spawnPoints.push_back(
          {typeName, x, y});

      trySpawnAround(typeName, x, y);
    }
  }

  std::cout
      << "[SpawnManager] "
      << spawnPoints.size()
      << " spawn points cargados"
      << std::endl;
}

std::optional<uint32_t> SpawnManager::spawnNpc(
    const std::string &typeName,
    int tileX,
    int tileY)
{
  if (!collision.isWalkableTile(tileX, tileY))
    return std::nullopt;

  
  float px = static_cast<float>(tileX * tileSize + tileSize / 2);
  float py = static_cast<float>(tileY * tileSize + tileSize / 2);

  uint32_t npcId =
      npcManager.spawnNpc(
          typeName,
          px,
          py);

  return npcId;
}

std::optional<uint32_t> SpawnManager::trySpawnAround(
    const std::string &typeName,
    int x,
    int y)
{
  for (int attempts = 0; attempts < 10; attempts++)
  {
    int tx = x + (std::rand() % 7) - 3;
    int ty = y + (std::rand() % 7) - 3;

    if (tx < 0 || ty < 0)  
      continue;

    if (collision.isWalkableTile(tx, ty))
    {
      auto id = spawnNpc(typeName, tx, ty);

      if (id)
      {
        std::cout
            << "[SpawnManager] spawneado "
            << typeName
            << " en ("
            << tx
            << ", "
            << ty
            << ")"
            << std::endl;
      }

      return id;
    }
  }

  return std::nullopt;
}

std::vector<uint32_t> SpawnManager::tick()
{
  std::vector<uint32_t> spawned;

  spawnTickCounter++;

  if (spawnTickCounter < spawnEveryNTicks)
    return spawned;

  spawnTickCounter = 0;

  int toSpawn =
      std::min(
          spawnBatchSize,
          maxNpcs - npcManager.count());

  for (int i = 0;
       i < toSpawn && !spawnPoints.empty();
       i++)
  {
    auto &point =
        spawnPoints[
            std::rand() % spawnPoints.size()];

    auto id =
        trySpawnAround(
            point.typeName,
            point.x,
            point.y);

    if (id)
      spawned.push_back(*id);
  }

  return spawned;
}
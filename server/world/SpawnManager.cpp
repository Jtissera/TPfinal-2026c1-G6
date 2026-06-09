#include "SpawnManager.h"
#include <cstdlib>
#include <iostream>

SpawnManager::SpawnManager(const toml::table &config, NpcManager &npcManager,
                           const CollisionSystem &collision,
                           OccupancySystem &occupancy)
    : npcManager(npcManager), collision(collision), occupancy(occupancy),
      spawnEveryNTicks(config["npc"]["spawn_interval_ticks"].value_or(200)),
      maxNpcs(config["npc"]["max_population"].value_or(20)),
      spawnBatchSize(config["npc"]["spawn_batch_size"].value_or(4)) {}

std::optional<uint32_t> SpawnManager::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
  if (!collision.isWalkable(tileX, tileY))
  {
    return std::nullopt;
  }

  if (occupancy.isOccupied(tileX, tileY))
  {
    return std::nullopt;
  }

  const uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);
  occupancy.occupy(tileX, tileY, npcId);

  return npcId;
}

std::optional<uint32_t> SpawnManager::trySpawnAround(const std::string &typeName,
                                                     int x,
                                                     int y)
{
  for (int attempts = 0; attempts < 10; ++attempts)
  {
    const int dx = (std::rand() % 7) - 3;
    const int dy = (std::rand() % 7) - 3;

    const int tx = x + dx;
    const int ty = y + dy;

    if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
    {
      return spawnNpc(typeName, tx, ty);
    }
  }

  return std::nullopt;
}

std::vector<uint32_t> SpawnManager::tick()
{
  std::vector<uint32_t> spawnedNpcIds;

  spawnTickCounter++;

  if (spawnTickCounter < spawnEveryNTicks)
  {
    return spawnedNpcIds;
  }

  spawnTickCounter = 0;

  const int currentNpcs = static_cast<int>(npcManager.count());
  const int remainingCapacity = maxNpcs - currentNpcs;

  std::cout << "[SPAWN MANAGER] tick currentNpcs="
            << currentNpcs
            << " maxNpcs="
            << maxNpcs
            << " remainingCapacity="
            << remainingCapacity
            << " spawnPoints="
            << spawnPoints.size()
            << std::endl;

  if (remainingCapacity <= 0)
  {
    std::cout << "[SPAWN MANAGER] no spawnea: capacidad llena"
              << std::endl;
    return spawnedNpcIds;
  }

  const int toSpawn = std::min(spawnBatchSize, remainingCapacity);

  for (int i = 0; i < toSpawn && !spawnPoints.empty(); ++i)
  {
    const auto &point = spawnPoints[std::rand() % spawnPoints.size()];

    auto npcId = trySpawnAround(point.typeName, point.x, point.y);

    if (npcId.has_value())
    {
      std::cout << "[SPAWN MANAGER] spawned npcId="
                << npcId.value()
                << " type="
                << point.typeName
                << std::endl;

      spawnedNpcIds.push_back(npcId.value());
    }
    else
    {
      std::cout << "[SPAWN MANAGER] fallo spawn type="
                << point.typeName
                << " base=("
                << point.x
                << ", "
                << point.y
                << ")"
                << std::endl;
    }
  }

  return spawnedNpcIds;
}

void SpawnManager::loadSpawnPoints(const MapData &mapData)
{
  // Loop existente para enemigos
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

      spawnPoints.push_back({typeName,
                             static_cast<int>(x),
                             static_cast<int>(y)});

      (void)trySpawnAround(typeName, static_cast<int>(x), static_cast<int>(y));
    }
  }

  // Loop para NPCs de ciudad, spawn exacto sin desplazamiento
  for (uint16_t y = 0; y < mapData.height(); y++)
  {
    for (uint16_t x = 0; x < mapData.width(); x++)
    {
      const Tile &tile = mapData.at(x, y);

      if (tile.npc != NpcType::PRIEST &&
          tile.npc != NpcType::MERCHANT &&
          tile.npc != NpcType::BANKER)
        continue;

      std::string typeName = npcTypeKey(tile.npc);
      if (typeName.empty())
        continue;

      spawnNpc(typeName, static_cast<int>(x), static_cast<int>(y));
    }
  }
}
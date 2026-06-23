#include "NpcSpawner.h"

namespace
{
    bool isSpawnableCombatNpc(NpcType type)
    {
        return type != NpcType::NONE &&
               type != NpcType::PRIEST &&
               type != NpcType::MERCHANT &&
               type != NpcType::BANKER;
    }

    bool isCityNpc(NpcType type)
    {
        return type == NpcType::PRIEST ||
               type == NpcType::MERCHANT ||
               type == NpcType::BANKER;
    }
}

NpcSpawner::NpcSpawner(NpcManager &npcManager,
                       OccupancySystem &occupancy,
                       const CollisionSystem &collision,
                       const toml::table &config)
    : npcManager(npcManager),
      occupancy(occupancy),
      collision(collision),
      tileSize(config["world"]["tile_size"].value_or(96)),
      npcRespawnDelayMs(config["npc"]["respawn_ms"].value_or(5000.0f)),
      spawnEveryNTicks(config["npc"]["spawn_interval_ticks"].value_or(200)),
      maxNpcs(config["npc"]["max_population"].value_or(20)),
      spawnBatchSize(config["npc"]["spawn_batch_size"].value_or(4)),
      spawnTickCounter(0)
{
}

std::optional<std::pair<int, int>> NpcSpawner::findAndOccupyAdjacentTile(
    int tileX, int tileY, uint32_t entityId)
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (dx == 0 && dy == 0)
                continue;

            const int tx = tileX + dx;
            const int ty = tileY + dy;

            if (collision.isWalkable(tx, ty) && occupancy.occupy(tx, ty, entityId))
                return std::make_pair(tx, ty);
        }
    }
    return std::nullopt;
}

void NpcSpawner::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    if (!collision.isWalkable(tileX, tileY) || occupancy.isOccupied(tileX, tileY))
        return;

    const uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);
    occupancy.occupy(tileX, tileY, npcId);
}

std::optional<uint32_t> NpcSpawner::trySpawnAround(const std::string &typeName, int x, int y)
{
    for (int attempts = 0; attempts < 10; ++attempts)
    {
        const int dx = (std::rand() % 7) - 3;
        const int dy = (std::rand() % 7) - 3;
        const int tx = x + dx;
        const int ty = y + dy;

        if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
        {
            const uint32_t npcId = npcManager.spawnNpc(typeName, tx, ty);
            occupancy.occupy(tx, ty, npcId);
            return npcId;
        }
    }
    return std::nullopt;
}

void NpcSpawner::spawnMapNpcs(const MapData &mapData)
{
    spawnCombatNpcs(mapData);
    spawnCityNpcs(mapData);
}

void NpcSpawner::spawnCombatNpcs(const MapData &mapData)
{
    for (uint16_t y = 0; y < mapData.height(); ++y)
    {
        for (uint16_t x = 0; x < mapData.width(); ++x)
        {
            const Tile &tile = mapData.at(x, y);

            if (!isSpawnableCombatNpc(tile.npc))
                continue;

            const std::string typeName = npcTypeKey(tile.npc);
            if (typeName.empty())
                continue;

            spawnPoints.push_back({typeName, static_cast<int>(x), static_cast<int>(y)});
            trySpawnAround(typeName, static_cast<int>(x), static_cast<int>(y));
        }
    }
}

void NpcSpawner::spawnCityNpcs(const MapData &mapData)
{
    for (uint16_t y = 0; y < mapData.height(); ++y)
    {
        for (uint16_t x = 0; x < mapData.width(); ++x)
        {
            const Tile &tile = mapData.at(x, y);

            if (!isCityNpc(tile.npc))
                continue;

            const std::string typeName = npcTypeKey(tile.npc);
            if (typeName.empty())
                continue;

            spawnNpc(typeName, static_cast<int>(x), static_cast<int>(y));
        }
    }
}

void NpcSpawner::startNpcRespawn(uint32_t npcId)
{
    npcManager.startRespawn(npcId, npcRespawnDelayMs);
}


bool NpcSpawner::placeRespawnedNpc(Npc &npc, WorldTickResult &result)
{
    const int tx = npc.getSpawnTileX();
    const int ty = npc.getSpawnTileY();

    if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
    {
        occupancy.occupy(tx, ty, npc.getId());
        npc.respawn();
    }
    else
    {
        const std::optional<std::pair<int, int>> freeTile =
            findAndOccupyAdjacentTile(tx, ty, npc.getId());

        if (!freeTile)
            return false;

        npc.respawn();
        npc.setTilePos(freeTile->first, freeTile->second);
    }

    result.spawnedNpcs.push_back({npc.getId(),
                                  npc.getType(),
                                  npc.getName(),
                                  static_cast<uint16_t>(npc.getTileX() * tileSize),
                                  static_cast<uint16_t>(npc.getTileY() * tileSize),
                                  static_cast<uint16_t>(npc.getHp()),
                                  static_cast<uint16_t>(npc.getMaxHp()),
                                  static_cast<uint16_t>(npc.getStats().level),
                                  npc.isHostile()});
    return true;
}

void NpcSpawner::processRespawns(float deltaMs, WorldTickResult &result)
{
    const std::vector<uint32_t> respawnedIds = npcManager.tickRespawns(deltaMs);

    for (uint32_t npcId : respawnedIds)
    {
        Npc *npc = npcManager.findNpc(npcId);
        if (npc == nullptr)
            continue;

        if (!placeRespawnedNpc(*npc, result))
            startNpcRespawn(npcId);
    }
}
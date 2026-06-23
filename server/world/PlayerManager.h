#pragma once

#include "../game/player/Player.h"
#include "OccupancySystem.h"
#include "CollisionSystem.h"
#include "../game/stats/gameFormulas.h"
#include "../game/items/itemRepository.h"
#include "GroundManager.h"
#include "../resurrection/resurrectionSystem.h"
#include "../game/clan/clanManager.h"
#include "../../common/dtos/gameTypes.h"
#include "WorldTickResult.h"
#include "DeathResult.h"

#include <toml++/toml.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class PlayerManager
{
public:
    PlayerManager(OccupancySystem &occupancy,
                  const CollisionSystem &collision,
                  const GameFormulas &formulas,
                  ItemRepository &itemRepo,
                  ResurrectionSystem &resurrectionSystem,
                  ClanManager &clanManager,
                  const toml::table &config);

    void addPlayer(Player player);
    std::optional<Player> removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);

    Player &getPlayer(uint32_t id);
    const Player &getPlayer(uint32_t id) const;

    bool hasPlayer(uint32_t id) const;
    bool canPlayerAct(uint32_t id) const;

    int getTileX(uint32_t id) const;
    int getTileY(uint32_t id) const;
    int getPixelX(uint32_t id) const;
    int getPixelY(uint32_t id) const;

    void giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier = 1.0f);

    DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId,
                                  GroundManager &groundManager);

    void resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY);
    void handleResurrectionComplete(uint32_t playerId, int tileX, int tileY,
                                    WorldTickResult &result);

    void tickPlayers(float deltaSeconds, const MapData &mapData, WorldTickResult &result);

    std::optional<uint32_t> findPlayerIdByName(const std::string &name) const;
    int countClanAlliesNear(const Player &player, int radiusTiles) const;
    std::vector<uint32_t> getOnlineClanMemberIds(const std::string &clanName) const;

    const std::unordered_map<uint32_t, Player> &getPlayers() const;

private:
    void loadInitialInventoryForPlayer(Player &player);
    void tickPlayerStats(float deltaSeconds, WorldTickResult &result);
    void checkPlayerTileEvents(const MapData &mapData, WorldTickResult &result);
    std::optional<std::pair<int, int>> findAndOccupyAdjacentTile(int tileX, int tileY,
                                                                   uint32_t entityId);

    OccupancySystem &occupancy;
    const CollisionSystem &collision;
    const GameFormulas &formulas;
    ItemRepository &itemRepo;
    ResurrectionSystem &resurrectionSystem;
    ClanManager &clanManager;

    std::unordered_map<uint32_t, Player> players;

    int tileSize;
    float playerMoveStep;
};
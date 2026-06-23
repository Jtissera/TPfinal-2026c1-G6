#pragma once

#include "PlayerManager.h"
#include "NpcSpawner.h"
#include "WorldTickResult.h"
#include "../../common/dtos/gameTypes.h"
#include "../../editor/map/mapData.h"
#include "../../editor/map/mapSerializer.h"
#include "../game/items/itemRepository.h"
#include "../game/player/Player.h"
#include "../game/stats/gameFormulas.h"
#include "../npc/npcFactory.h"
#include "../npc/npcManager.h"
#include "CollisionSystem.h"
#include "GroundManager.h"
#include "OccupancySystem.h"
#include "SpawnManager.h"
#include "../bank/bankRepository.h"
#include "../resurrection/resurrectionSystem.h"
#include "../city/priestHandler.h"
#include "../city/merchantHandler.h"
#include "../city/bankerHandler.h"
#include "../city/cityNpcDispatcher.h"
#include "server/game/clan/clanManager.h"

#include <toml++/toml.h>

#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <algorithm>

class GameWorld
{
public:
    GameWorld(const std::string &mapPath,
              NpcFactory &npcFactory,
              ItemRepository &itemRepo,
              const toml::table &config,
              ClanManager &clanManager);

    GameWorld(MapData mapData,
              NpcFactory &npcFactory,
              ItemRepository &itemRepo,
              const toml::table &config,
              ClanManager &clanManager);

    void addPlayer(Player player);
    std::optional<Player> removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);

    Player &getPlayer(uint32_t id);
    const Player &getPlayer(uint32_t id) const;

    bool hasPlayer(uint32_t playerId) const;
    bool canPlayerAct(uint32_t id) const;

    int getTileX(uint32_t id) const;
    int getTileY(uint32_t id) const;
    int getPixelX(uint32_t id) const;
    int getPixelY(uint32_t id) const;

    void giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier = 1.0f);
    DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId);

    std::optional<uint32_t> findPlayerIdByName(const std::string &name) const;
    int countClanAlliesNear(const Player &player, int radiusTiles) const;
    std::vector<uint32_t> getOnlineClanMemberIds(const std::string &clanName) const;

    const std::unordered_map<uint32_t, Player> &getPlayers() const;

    void spawnMapNpcs();
    void spawnNpc(const std::string &typeName, int tileX, int tileY);
    bool hasNpc(uint32_t npcId) const;
    bool damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId);
    NpcDropResult handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId);
    Npc &getNpc(uint32_t npcId);
    const Npc &getNpc(uint32_t npcId) const;
    const std::unordered_map<uint32_t, Npc> &getNpcs() const;
    std::optional<NpcType> getNpcTypeAtTile(int tileX, int tileY) const;

    void addItemOnGround(Item item, int tileX, int tileY);
    uint32_t addGoldOnGround(uint32_t amount, int tileX, int tileY);
    std::optional<Item> pickItemById(uint32_t instanceId);
    std::optional<uint32_t> pickGoldById(uint32_t instanceId);
    const GroundManager &getGroundManager() const;

    const MapData &getMapData() const;
    const Tile &getTileAt(int tileX, int tileY) const;
    std::pair<int, int> findSafeSpawnNear(int tileX, int tileY) const;

    CityResult handleCityInteraction(uint32_t playerId, NpcType npcType,
                                     const CityCommand &cmd);
    CityResult handleRemoteResurrect(uint32_t playerId);

    WorldTickResult tick(float deltaSeconds);

    void resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY);

private:
    void tickNpcs(WorldTickResult &result);
    void resolveNpcMovement(NpcTickResult &npcResult, WorldTickResult &result);
    void resolveNpcAttacks(NpcTickResult &npcResult, WorldTickResult &result);
    void resolveNpcDeaths(NpcTickResult &npcResult, WorldTickResult &result);

    MapData mapData;
    CollisionSystem collision;
    OccupancySystem occupancy;
    GameFormulas formulas;
    NpcManager npcManager;
    ItemRepository &itemRepo;
    ClanManager &clanManager;
    BankRepository bankRepo;
    ResurrectionSystem resurrectionSystem;

    PriestHandler priestHandler;
    MerchantHandler merchantHandler;
    BankerHandler bankerHandler;
    CityNpcDispatcher cityDispatcher;

    GroundManager groundManager;

    PlayerManager playerManager;
    NpcSpawner npcSpawner;

    std::vector<WorldTickResult::ResurrectStartedInfo> pendingResurrectionStarts;
};
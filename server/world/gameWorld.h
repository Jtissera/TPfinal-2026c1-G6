#pragma once

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
#include <iostream>
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
  explicit GameWorld(const std::string &mapPath, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config,
                     ClanManager &clanManager);

  explicit GameWorld(MapData mapData, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config,
                     ClanManager &clanManager);

  struct InstanceEntry
  {
    uint32_t playerId;
    std::string targetMap;
    int returnTileX;
    int returnTileY;
  };
  struct NpcSpawnEvent
  {
    uint32_t npcId;
    NpcType type;
    std::string name;
    uint16_t x;
    uint16_t y;
    uint16_t hp;
    uint16_t maxHp;
    uint16_t level;
    bool hostile;
  };
  struct DeathResult
  {
    uint32_t excessGold;
    uint32_t goldInstanceId = 0;
    std::vector<Item> droppedItems;
    int tileX = 0;
    int tileY = 0;
  };
  struct WorldTickResult
  {
    std::vector<uint32_t> playersDied;
    std::vector<uint32_t> playersChanged;
    std::vector<uint32_t> npcsMoved;
    std::vector<NpcDeathResult> npcDeaths;

    struct PlayerHit
    {
      uint32_t playerId;
      int16_t damage;
    };
    std::vector<PlayerHit> playerHits;
    std::vector<InstanceEntry> instanceTransitions;
    std::vector<NpcSpawnEvent> spawnedNpcs;

    std::vector<std::pair<uint32_t, DeathResult>> playerDeathsByNpc;

    struct PlayerResurrection
    {
      uint32_t playerId;
      uint16_t tileX;
      uint16_t tileY;
    };

    struct ResurrectStartedInfo
    {
      uint32_t playerId;
      uint32_t delayMs;
    };
    std::vector<PlayerResurrection> playersResurrected;
    std::vector<ResurrectStartedInfo> resurrectionStarted;

    struct ClanAllyHit
    {
      std::string clanName;
      std::string targetName;
      uint32_t targetId;
    };
    std::vector<ClanAllyHit> clanAllyHits;

    struct NpcAttackAnim
    {
      uint32_t npcId;
      Direction direction;
    };

    std::vector<NpcAttackAnim> npcAttacksForAnim;
  };

  void addPlayer(Player player);
  std::optional<Player> removePlayer(uint32_t id);
  bool movePlayer(uint32_t id, Direction dir);

  Player &getPlayer(uint32_t id);
  const Player &getPlayer(uint32_t id) const;

  bool canPlayerAct(uint32_t id) const;

  int getTileX(uint32_t id) const;
  int getTileY(uint32_t id) const;
  int getPixelX(uint32_t id) const;
  int getPixelY(uint32_t id) const;

  const Tile &getTileAt(int tileX, int tileY) const;

  void giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier = 1.0f);
  DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId);

  void addItemOnGround(Item item, int tileX, int tileY);
  std::optional<Item> pickItemById(uint32_t instanceId);

  uint32_t addGoldOnGround(uint32_t amount, int tileX, int tileY);
  std::optional<uint32_t> pickGoldById(uint32_t instanceId);
  const GroundManager &getGroundManager() const;

  void spawnNpc(const std::string &typeName, int tileX, int tileY);

  WorldTickResult tick(float deltaSeconds);

  const MapData &getMapData() const;
  const std::unordered_map<uint32_t, Npc> &getNpcs() const;

  void resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY);
  bool hasNpc(uint32_t npcId) const;
  bool damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId);

  NpcDropResult handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId);
  bool hasPlayer(uint32_t playerId) const;

  std::optional<uint32_t> findPlayerIdByName(const std::string &name) const;

  Npc &getNpc(uint32_t npcId);
  const Npc &getNpc(uint32_t npcId) const;

  const std::unordered_map<uint32_t, Player> &getPlayers() const;
  std::pair<int, int> findSafeSpawnNear(int tileX, int tileY) const;

  CityResult handleCityInteraction(uint32_t playerId, NpcType npcType,
                                   const CityCommand &cmd);
  CityResult handleRemoteResurrect(uint32_t playerId);
  std::optional<NpcType> getNpcTypeAtTile(int tileX, int tileY) const;

  int countClanAlliesNear(const Player &player, int radiusTiles) const;
  std::vector<uint32_t> getOnlineClanMemberIds(const std::string &clanName) const;

private:
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

  std::unordered_map<uint32_t, Player> players;
  GroundManager groundManager;
  SpawnManager spawnManager;

  std::vector<WorldTickResult::ResurrectStartedInfo> pendingResurrectionStarts;

  int tileSize;
  float npcRespawnDelayMs;
  float playerMoveStep;

  std::vector<std::pair<std::string, std::pair<int, int>>> spawnPoints;

  std::optional<std::pair<int, int>> findAndOccupyAdjacentTile(int tileX, int tileY,
                                                                uint32_t entityId);

  void handleResurrectionComplete(uint32_t playerId, int tileX, int tileY,
                                  WorldTickResult &result);
  void processNpcRespawns(float deltaMs, WorldTickResult &result);
  bool placeRespawnedNpc(Npc &npc, WorldTickResult &result);

  void tickPlayers(float deltaSeconds, WorldTickResult &result);
  void tickNpcs(WorldTickResult &result);
  void resolveNpcMovement(NpcTickResult &npcResult, WorldTickResult &result);
  void resolveNpcAttacks(NpcTickResult &npcResult, WorldTickResult &result);
  void resolveNpcDeaths(NpcTickResult &npcResult, WorldTickResult &result);

  void spawnMapNpcs();
  void spawnCombatNpcsFromMap();
  void spawnCityNpcsFromMap();

  void loadInitialInventoryForPlayer(Player &player);
};
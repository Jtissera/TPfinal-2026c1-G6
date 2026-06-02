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
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class GameWorld {
public:
  explicit GameWorld(const std::string &mapPath, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config);

  explicit GameWorld(MapData mapData, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config);

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

  void giveExperience(uint32_t playerId, uint32_t exp);

  struct DeathResult {
    uint32_t excessGold;
    std::vector<Item> droppedItems;
  };
  DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId);

  void addItemOnGround(Item item, int tileX, int tileY);
  std::optional<Item> pickItemAt(int tileX, int tileY);

  void addGoldOnGround(uint32_t amount, int tileX, int tileY);
  std::optional<uint32_t> pickGoldAt(int tileX, int tileY);

  void spawnNpc(const std::string &typeName, int tileX, int tileY);

  // Tick para actualizar
  struct WorldTickResult {
    std::vector<uint32_t> playersChanged;
    std::vector<uint32_t> npcsMoved;
    std::vector<NpcDeathResult> npcDeaths;
    std::vector<uint32_t> npcSpawned; 
    struct PlayerHit {
      uint32_t playerId;
      int16_t damage;
    };
    std::vector<PlayerHit> playerHits;
  };

  const Npc& getNpc(uint32_t id) const;
  WorldTickResult tick(float deltaSeconds);

  const MapData &getMapData() const { return mapData; }
  const std::unordered_map<uint32_t, Npc> &getNpcs() const;

  void resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY);

private:
  void tickPlayers(float deltaSeconds, WorldTickResult &result);
  void tickNpcs(WorldTickResult &result);

  MapData mapData;
  CollisionSystem collision;
  OccupancySystem occupancy;
  GameFormulas formulas;
  NpcManager npcManager;
  ItemRepository &itemRepo;

  std::unordered_map<uint32_t, Player> players;
  GroundManager groundManager;
  SpawnManager spawnManager;
  int tileSize;
};
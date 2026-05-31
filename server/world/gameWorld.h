#pragma once

#include "../game/Player.h"
#include "../game/gameFormulas.h"
#include "../npc/npcManager.h"
#include "../npc/npcFactory.h"
#include "../game/itemRepository.h"
#include "../../editor/map/mapData.h"
#include "../../editor/map/mapSerializer.h"
#include "../../common/dtos/gameTypes.h"
#include "CollisionSystem.h"
#include "OccupancySystem.h"
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdexcept>
#include <iostream>

class GameWorld {
public:
    explicit GameWorld(const std::string& mapPath,
                       NpcFactory& npcFactory,
                       ItemRepository& itemRepo);

    explicit GameWorld(MapData mapData,
                       NpcFactory& npcFactory,
                       ItemRepository& itemRepo);


    void addPlayer(Player player);
    void removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);

    Player&       getPlayer(uint32_t id);
    const Player& getPlayer(uint32_t id) const;

    bool canPlayerAct(uint32_t id) const;

    int getTileX(uint32_t id)  const;
    int getTileY(uint32_t id)  const;
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


    void spawnNpc(const std::string& typeName, int tileX, int tileY);

    // Tick para actualizar
    struct WorldTickResult {
        std::vector<uint32_t> playersChanged;
        std::vector<uint32_t> npcsMoved;
        std::vector<NpcDeathResult> npcDeaths;
        struct PlayerHit {
            uint32_t playerId;
            int16_t  damage;
        };
        std::vector<PlayerHit> playerHits;
    };
    WorldTickResult tick(float deltaSeconds);

    const MapData& getMapData() const { return mapData; }
    const std::unordered_map<uint32_t, Npc>& getNpcs() const;

    void resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY);
    bool hasNpc(uint32_t npcId) const;
    bool damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId);
    bool hasPlayer(uint32_t playerId) const;
    Npc& getNpc(uint32_t npcId);
    const Npc& getNpc(uint32_t npcId) const;

private:
    static constexpr int TILE_SIZE = 96; //toml

    MapData         mapData;
    CollisionSystem collision;
    OccupancySystem occupancy;
    GameFormulas    formulas;
    NpcManager      npcManager;
    ItemRepository& itemRepo;

    std::unordered_map<uint32_t, Player> players;

    struct GroundItem {
        Item item;
        int tileX, tileY;
    };
    struct GroundGold {
        uint32_t amount;
        int tileX, tileY;
    };

    std::vector<GroundItem> groundItems;
    std::vector<GroundGold> groundGold;

    void spawnMapNpcs();

    int spawnTickCounter = 0;
    static constexpr int SPAWN_EVERY_N_TICKS = 200;
    static constexpr int MAX_NPCS            = 20;  // a TOML
    static constexpr int SPAWN_BATCH_SIZE    = 4;   // a TOML

    std::vector<std::pair<std::string, std::pair<int,int>>> spawnPoints;
};
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
#include "SpawnManager.h"
#include "../bank/bankRepository.h"
#include "../resurrection/resurrectionSystem.h"
#include "../city/priestHandler.h"
#include "../city/merchantHandler.h"
#include "../city/bankerHandler.h"
#include "../city/cityNpcDispatcher.h"
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

// OccupancySystem eliminado. La colision entre entidades se resuelve
// con AABB usando CollisionSystem::overlaps() en movePlayer/tickNpcs.

class GameWorld {
public:
    explicit GameWorld(const std::string& mapPath,
                       NpcFactory& npcFactory,
                       ItemRepository& itemRepo,
                       const toml::table& config);

    explicit GameWorld(MapData mapData,
                       NpcFactory& npcFactory,
                       ItemRepository& itemRepo,
                       const toml::table& config);

    struct InstanceEntry {
        uint32_t    playerId;
        std::string targetMap;
        int         returnTileX;
        int         returnTileY;
    };

    struct WorldTickResult {
        std::vector<uint32_t>       playersChanged;
        std::vector<uint32_t>       npcsMoved;
        std::vector<NpcDeathResult> npcDeaths;
        std::vector<uint32_t>       npcSpawned;

        struct PlayerHit {
            uint32_t playerId;
            int16_t  damage;
        };
        std::vector<PlayerHit>    playerHits;
        std::vector<InstanceEntry> instanceTransitions;
    };

    void                  addPlayer(Player player);
    std::optional<Player> removePlayer(uint32_t id);

    // Intenta mover al jugador en la direccion dada.
    // Valida colision con mapa y con otras entidades (AABB).
    // Devuelve true si el movimiento fue exitoso.
    bool movePlayer(uint32_t id, Direction dir);

    Player&       getPlayer(uint32_t id);
    const Player& getPlayer(uint32_t id) const;

    bool canPlayerAct(uint32_t id) const;

    // Helpers de posicion
    int   getTileX(uint32_t id)  const;
    int   getTileY(uint32_t id)  const;
    float getPixelX(uint32_t id) const;
    float getPixelY(uint32_t id) const;

    // Pixel del centro del tile para broadcast al cliente
    int getPixelXForBroadcast(uint32_t id) const;
    int getPixelYForBroadcast(uint32_t id) const;

    const Tile& getTileAt(int tileX, int tileY) const;

    void giveExperience(uint32_t playerId, uint32_t exp,
                        float xpMultiplier = 1.0f);

    struct DeathResult {
        uint32_t          excessGold;
        std::vector<Item> droppedItems;
    };
    DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId);

    void                  addItemOnGround(Item item, int tileX, int tileY);
    std::optional<Item>   pickItemAt(int tileX, int tileY);

    void                  addGoldOnGround(uint32_t amount, int tileX, int tileY);
    std::optional<uint32_t> pickGoldAt(int tileX, int tileY);

    void spawnNpc(const std::string& typeName, int tileX, int tileY);

    const Npc& getNpc(uint32_t id) const;
    const std::unordered_map<uint32_t, Npc>& getNpcs() const;

    WorldTickResult tick(float deltaSeconds);

    const MapData& getMapData() const { return mapData; }

    void resurrectPlayer(uint32_t id, float pixelX, float pixelY);
    std::pair<float, float> findSafeSpawnNear(int tileX, int tileY) const;

    CityResult handleCityInteraction(uint32_t playerId,
                                     NpcType npcType,
                                     const CityCommand& cmd);
    CityResult handleRemoteResurrect(uint32_t playerId);

    std::optional<NpcType> getNpcTypeAtTile(int tileX, int tileY) const;

    // Chequea adyacencia entre jugador y un punto en pixeles
    // (para interaccion con NPC de ciudad, pickup, etc.)
    bool isPlayerAdjacentTo(uint32_t playerId,
                             float targetPixelX, float targetPixelY) const;

    bool isMoveWalkable(float px, float py) const;

private:
    void tickPlayers(float deltaSeconds, WorldTickResult& result);
    void tickNpcs(WorldTickResult& result);

    // Colision entre entidades: devuelve true si el hitbox en (px,py)
    // solapa con algun otro jugador o NPC (excluyendo excludeId)
    bool entityCollides(float px, float py,
                        float hitboxW, float hitboxH,
                        uint32_t excludeId) const;

    Rect playerHitbox(float px, float py) const;
    Rect npcHitbox(float px, float py)    const;

    MapData         mapData;
    CollisionSystem collision;
    GameFormulas    formulas;
    NpcManager      npcManager;
    ItemRepository& itemRepo;
    BankRepository  bankRepo;
    ResurrectionSystem resurrectionSystem;
    PriestHandler   priestHandler;
    MerchantHandler merchantHandler;
    BankerHandler   bankerHandler;
    CityNpcDispatcher cityDispatcher;

    std::unordered_map<uint32_t, Player> players;
    GroundManager groundManager;
    SpawnManager  spawnManager;

    int   tileSize;
    float playerHitboxW;
    float playerHitboxH;
    float npcHitboxW;
    float npcHitboxH;
    float adjacencyThreshold;
};
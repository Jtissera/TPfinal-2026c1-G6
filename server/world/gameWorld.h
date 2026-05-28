#pragma once

#include "../game/Player.h"
#include "../game/gameFormulas.h"
#include "../../editor/map/mapData.h"
#include "../../editor/map/mapSerializer.h"
#include "../../common/dtos/gameTypes.h"
#include "CollisionSystem.h"
#include <unordered_map>
#include <vector>

class GameWorld {
public:

    struct DeathResult {
        uint32_t excessGold;
        std::vector<Item> droppedItems;
    };
    
    explicit GameWorld(const std::string& mapPath);
    explicit GameWorld(MapData mapData);//TESTEO

    void addPlayer(Player player);
    void removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);

    int getX(uint32_t id) const;
    int getY(uint32_t id) const;

    std::vector<uint32_t> tick(float deltaSeconds);
    Player& getPlayer(uint32_t id);

    void addItemOnGround(Item item, int x, int y);
    std::optional<Item> pickItemAt(int x, int y);

    DeathResult handlePlayerDeath(uint32_t targetId, uint32_t attackerId);
    std::optional<uint32_t> pickGoldAt(int x, int y);

private:
    static constexpr int SPEED = 10;

    MapData         mapData;
    CollisionSystem collision;
    std::unordered_map<uint32_t, Player> players;
    GameFormulas    formulas;

    struct GroundItem {
        Item item;
        int x, y;
    };


    struct GroundGold {
    uint32_t amount;
    int x, y;
};

    std::vector<GroundItem> groundItems;
    std::vector<GroundGold> groundGold;

};
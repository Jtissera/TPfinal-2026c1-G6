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
    explicit GameWorld(const std::string& mapPath);
    explicit GameWorld(MapData mapData);

    void addPlayer(Player player);
    void removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);

    int getX(uint32_t id) const;
    int getY(uint32_t id) const;

    std::vector<uint32_t> tick(float deltaSeconds);
    const Player& getPlayer(uint32_t id) const;

private:
    static constexpr int SPEED = 10;

    MapData         mapData;
    CollisionSystem collision;
    std::unordered_map<uint32_t, Player> players;
    GameFormulas    formulas;
};
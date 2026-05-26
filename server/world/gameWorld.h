#pragma once
#include "../game/Player.h"
#include "../../editor/map/mapData.h"
#include "../../editor/map/mapSerializer.h"
#include "../../common/dtos/gameTypes.h"
#include <unordered_map>

class GameWorld {
public:
    explicit GameWorld(const std::string& mapPath);
    void addPlayer(Player player);
    void removePlayer(uint32_t id);
    bool movePlayer(uint32_t id, Direction dir);
    int getX(uint32_t id) const;
    int getY(uint32_t id) const;

private:
    static constexpr int SPEED          = 10;
    static constexpr int HITBOX_W       = 64;
    static constexpr int HITBOX_H       = 96;
    static constexpr int HITBOX_OFFSET_X = 32;
    static constexpr int HITBOX_OFFSET_Y = 32;

    bool wouldCollide(int x, int y) const;
    MapData mapData;
    std::unordered_map<uint32_t, Player> players;
};
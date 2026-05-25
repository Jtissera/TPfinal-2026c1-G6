#pragma once
#include "player.h"
#include "../../editor/map/mapData.h"
#include "../../editor/map/mapSerializer.h"
#include "../../common/dtos/gameTypes.h"
#include <unordered_map>

class GameWorld {
public:
    explicit GameWorld(const std::string& mapPath);

    void addPlayer(uint32_t id, int startX, int startY);
    void removePlayer(uint32_t id);

    // Retorna true si se movió (para saber si mandar update)
    bool movePlayer(uint32_t id, Direction dir);

    int getX(uint32_t id) const;
    int getY(uint32_t id) const;

private:
    bool wouldCollide(int x, int y) const;

    MapData mapData;
    std::unordered_map<uint32_t, Player> players;
};
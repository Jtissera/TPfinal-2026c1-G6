#pragma once
#include "Hitbox.h"
#include "../../editor/map/mapData.h"

class CollisionSystem {
public:
    explicit CollisionSystem(const MapData& mapData);

    bool wouldCollide(int x, int y, const Hitbox& hitbox) const;

    bool isInBounds(int x, int y, const Hitbox& hitbox) const;

private:
    static constexpr int TILE_SIZE = 96;
    const MapData& mapData;

    bool isTileBlocked(int px, int py) const;
};
#pragma once
#include "../../editor/map/mapData.h"
#include <cstdint>

class CollisionSystem {
public:
    explicit CollisionSystem(const MapData& mapData);

    bool isWalkable(int tileX, int tileY) const;
    bool isInBounds(int tileX, int tileY) const;

private:
    const MapData& mapData;
};
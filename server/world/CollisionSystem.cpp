#include "CollisionSystem.h"

CollisionSystem::CollisionSystem(const MapData& mapData)
    : mapData(mapData) {}

bool CollisionSystem::isInBounds(int tileX, int tileY) const {
    return tileX >= 0 && tileY >= 0 &&
           tileX < mapData.width() &&
           tileY < mapData.height();
}

bool CollisionSystem::isWalkable(int tileX, int tileY) const {
    if (!isInBounds(tileX, tileY)) return false;
    return mapData.at(tileX, tileY).walkable;
}
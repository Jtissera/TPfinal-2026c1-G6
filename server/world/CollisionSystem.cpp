#include "CollisionSystem.h"
#include <algorithm>
#include <cmath>

CollisionSystem::CollisionSystem(const MapData& mapData)
    : mapData(mapData), tileSize(96) {}

bool CollisionSystem::isInBoundsTile(int tileX, int tileY) const {
    return tileX >= 0 && tileY >= 0 &&
           tileX < mapData.width() &&
           tileY < mapData.height();
}

bool CollisionSystem::isWalkableTile(int tileX, int tileY) const {
    if (!isInBoundsTile(tileX, tileY)) return false;
    return mapData.at(tileX, tileY).walkable;
}

bool CollisionSystem::isInBounds(float pixelX, float pixelY) const {
    return isInBoundsTile(toTileX(pixelX), toTileY(pixelY));
}

bool CollisionSystem::isWalkable(float pixelX, float pixelY) const {
    return isWalkableTile(toTileX(pixelX), toTileY(pixelY));
}

bool CollisionSystem::overlaps(const Rect& a, const Rect& b) const {
    return a.x < b.x + b.w &&
           a.x + a.w > b.x &&
           a.y < b.y + b.h &&
           a.y + a.h > b.y;
}

bool CollisionSystem::isAdjacent(float ax, float ay,
                                  float bx, float by,
                                  float threshold) const {
    float dx = std::abs(ax - bx);
    float dy = std::abs(ay - by);
    return std::max(dx, dy) <= threshold;
}

int CollisionSystem::toTileX(float pixelX) const {
    return static_cast<int>(pixelX) / tileSize;
}

int CollisionSystem::toTileY(float pixelY) const {
    return static_cast<int>(pixelY) / tileSize;
}

float CollisionSystem::tileCenterX(int tileX) const {
    return static_cast<float>(tileX * tileSize + tileSize / 2);
}

float CollisionSystem::tileCenterY(int tileY) const {
    return static_cast<float>(tileY * tileSize + tileSize / 2);
}

int CollisionSystem::mapWidthTiles() const {
    return mapData.width();
}

int CollisionSystem::mapHeightTiles() const {
    return mapData.height();
}
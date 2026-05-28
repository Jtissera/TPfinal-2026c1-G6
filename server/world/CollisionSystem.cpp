#include "CollisionSystem.h"

CollisionSystem::CollisionSystem(const MapData& mapData)
    : mapData(mapData) {}

bool CollisionSystem::wouldCollide(int x, int y, const Hitbox& hitbox) const {
    int l = hitbox.left(x);
    int t = hitbox.top(y);
    int r = hitbox.right(x);
    int b = hitbox.bottom(y);

    return isTileBlocked(l, t) ||
           isTileBlocked(r, t) ||
           isTileBlocked(l, b) ||
           isTileBlocked(r, b);
}

bool CollisionSystem::isInBounds(int x, int y, const Hitbox& hitbox) const {
    if (hitbox.left(x)   < 0) return false;
    if (hitbox.top(y)    < 0) return false;
    if (hitbox.right(x)  >= mapData.width()  * TILE_SIZE) return false;
    if (hitbox.bottom(y) >= mapData.height() * TILE_SIZE) return false;
    return true;
}

bool CollisionSystem::isTileBlocked(int px, int py) const {
    if (px < 0 || py < 0) return true;
    int tx = px / TILE_SIZE;
    int ty = py / TILE_SIZE;
    if (tx >= mapData.width() || ty >= mapData.height()) return true;
    return !mapData.at(tx, ty).walkable;
}
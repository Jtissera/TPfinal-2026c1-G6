#include "OccupancySystem.h"

bool OccupancySystem::occupy(int tileX, int tileY, uint32_t entityId) {
    uint64_t k = key(tileX, tileY);

    if (occupants.count(k) > 0)
        return false;

    occupants[k] = entityId;
    return true;
}

void OccupancySystem::free(int tileX, int tileY) {
    occupants.erase(key(tileX, tileY));
}

bool OccupancySystem::isOccupied(int tileX, int tileY) const {
    return occupants.count(key(tileX, tileY)) > 0;
}

uint32_t OccupancySystem::getOccupant(int tileX, int tileY) const {
    auto it = occupants.find(key(tileX, tileY));
    if (it == occupants.end()) return 0;
    return it->second;
}

bool OccupancySystem::move(int fromX, int fromY, int toX, int toY, uint32_t entityId)
{
    if (fromX == toX && fromY == toY) {
        return true;
    }

    const uint32_t destinationOccupant = getOccupant(toX, toY);

    if (destinationOccupant != 0 && destinationOccupant != entityId) {
        return false;
    }

    free(fromX, fromY);
    occupants[key(toX, toY)] = entityId;

    return true;
}

uint64_t OccupancySystem::key(int tileX, int tileY) const {
    return (uint64_t)(uint32_t)tileX << KTileBits | (uint32_t)tileY;
}
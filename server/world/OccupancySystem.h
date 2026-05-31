#pragma once
#include <cstdint>
#include <unordered_map>

class OccupancySystem {
public:
    bool occupy(int tileX, int tileY, uint32_t entityId);
    void free(int tileX, int tileY);
    bool isOccupied(int tileX, int tileY) const;
    uint32_t getOccupant(int tileX, int tileY) const; 

    bool move(int fromX, int fromY, int toX, int toY, uint32_t entityId);

private:
    uint64_t key(int tileX, int tileY) const;
    std::unordered_map<uint64_t, uint32_t> occupants;
};
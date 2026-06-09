#pragma once

#include "common/npcType.h"
#include <cstdint>
#include <string>

enum class TileType : uint8_t
{
    GRASS = 0,
    WATER = 1,
    WALL = 2,
    FLOOR = 3,
    DOOR = 4,
    DUNGEON_ENTRANCE = 5,
    CAVERN_ENTRANCE = 6,
    EXIT = 7,
    SAND = 8,
    FOREST = 9,
    CACTUS = 10,
    STONE = 11,
    CITY_FLOOR = 12,
    HOUSE = 13,
    CHURCH = 14,
    MILL = 15
};

enum class ZoneType : uint8_t
{
    SAFE = 0,
    COMBAT = 1,
    CAVERN = 2,
    DUNGEON = 3,
    CITY = 4,
    DESERT = 5,
    FOREST = 6,
};

struct Tile
{
    TileType type = TileType::GRASS;
    ZoneType zone = ZoneType::COMBAT;
    bool walkable = true;
    NpcType npc = NpcType::NONE;
    std::string targetMap = "";
};
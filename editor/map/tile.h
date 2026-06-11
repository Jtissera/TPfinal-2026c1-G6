#pragma once

#include "common/npcType.h"
#include <cstdint>
#include <string>

enum class TileType : uint8_t
{
    GRASS = 0,
    WATER = 1,
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
    MILL = 15,
    CAVERN_FLOOR = 16,
    CAVERN_WALL_H = 17,
    CAVERN_WALL_V = 18,
    DUNGEON_FLOOR = 19,
    DUNGEON_WALL_H = 20,
    DUNGEON_WALL_V = 21,
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
#pragma once

#include <cstdint>
#include <string>
#include "common/npcType.h"

enum class TileType : uint8_t
{
    GRASS = 0,
    WATER = 1,
    WALL = 2,
    FLOOR = 3,
    DOOR = 4,
    DUNGEON_ENTRANCE = 5,
    CAVERN_ENTRANCE = 6,
    EXIT = 7, // salida de instancia, vuelve al mundo principal
};

enum class ZoneType : uint8_t
{
    SAFE = 0,
    COMBAT = 1,
    CAVERN = 2,
    DUNGEON = 3,
};

struct Tile
{
    TileType type = TileType::GRASS;
    ZoneType zone = ZoneType::SAFE;
    bool walkable = true;
    NpcType npc = NpcType::NONE;
    std::string targetMap = ""; // solo relevante para DUNGEON_ENTRANCE / CAVERN_ENTRANCE
};
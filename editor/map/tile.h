#pragma once

#include <cstdint>
#include "common/npcType.h"

// Tipos de tile del mapa
enum class TileType : uint8_t {
    GRASS     = 0,
    WATER     = 1,
    WALL      = 2,
    FLOOR     = 3,
    DOOR      = 4,
    DUNGEON_ENTRANCE = 5,
};

// Tipos de zona
enum class ZoneType : uint8_t {
    SAFE      = 0,   // ciudad/pueblo: no se puede atacar
    COMBAT    = 1,   // caverna/zona libre: se puede atacar
};

// Un tile del mapa
struct Tile {
    TileType  type     = TileType::GRASS;
    ZoneType  zone     = ZoneType::SAFE;
    bool      walkable = true;
    NpcType   npc      = NpcType::NONE;   // NPC fijo en este tile
};

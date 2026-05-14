#pragma once

#include <cstdint>

// Tipos de tile del mapa
enum class TileType : uint8_t {
    GRASS     = 0,
    WATER     = 1,
    WALL      = 2,
    FLOOR     = 3,
    DOOR      = 4,
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
    uint16_t  npcId    = 0;   // 0 = sin NPC
};

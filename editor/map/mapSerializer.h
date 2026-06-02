#pragma once

#include <string>
#include "mapData.h"
#include "common/npcType.h"

// Formato del archivo binario:
//   Header: "ARGMAP\0\0" (8 bytes magic)
//   uint16_t version     (2 bytes, little-endian)
//   uint16_t width       (2 bytes)
//   uint16_t height      (2 bytes)
//   uint16_t name_len    (2 bytes)
//   char[]   name        (name_len bytes, sin null)
//   Tiles:   width*height veces:
//     uint8_t  type
//     uint8_t  zone
//     uint8_t  walkable  (0 o 1)
//     uint8_t  npc      (NpcType)

class MapSerializer
{
public:
    // Guarda el mapa en un archivo binario. Lanza std::runtime_error si falla.
    static void save(const MapData &map, const std::string &filepath);

    // Carga el mapa desde un archivo binario. Lanza std::runtime_error si falla.
    static MapData load(const std::string &filepath);

private:
    static constexpr uint8_t MAGIC[8] = {'A', 'R', 'G', 'M', 'A', 'P', 0, 0};
    static constexpr uint16_t VERSION = 2;
};
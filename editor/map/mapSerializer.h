#pragma once

#include <string>
#include "mapData.h"

// Formato binario v3:
//   "ARGMAP\0\0"  (8 bytes magic)
//   uint16_t version    (2 bytes)  — ahora VERSION = 3
//   uint8_t  mapType    (1 byte)   — NUEVO en v3
//   uint16_t width      (2 bytes)
//   uint16_t height     (2 bytes)
//   uint16_t name_len   (2 bytes)
//   char[]   name       (name_len bytes, sin null)
//   Tiles: width*height veces:
//     uint8_t  type
//     uint8_t  zone
//     uint8_t  walkable (0 o 1)
//     uint8_t  npc
//     uint16_t targetMap_len
//     char[]   targetMap

class MapSerializer
{
public:
    static void save(const MapData &map, const std::string &filepath);
    static MapData load(const std::string &filepath);

private:
    static constexpr uint8_t MAGIC[8] = {'A', 'R', 'G', 'M', 'A', 'P', 0, 0};
    static constexpr uint16_t VERSION = 4;
};
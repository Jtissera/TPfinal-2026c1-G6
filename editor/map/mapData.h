#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "tile.h"

static constexpr uint16_t MAP_MAX_WIDTH = 200;
static constexpr uint16_t MAP_MAX_HEIGHT = 200;
static constexpr uint16_t MAP_MIN_SIZE = 5;

enum class MapType : uint8_t
{
    WORLD = 0,
    DUNGEON = 1,
    CAVE = 2,
};

class MapData
{
public:
    MapData();
    MapData(uint16_t width, uint16_t height,
            MapType type = MapType::WORLD);

    uint16_t width() const { return _width; }
    uint16_t height() const { return _height; }
    MapType mapType() const { return _mapType; }
    void setMapType(MapType t) { _mapType = t; }

    Tile &at(uint16_t x, uint16_t y);
    const Tile &at(uint16_t x, uint16_t y) const;
    bool inBounds(uint16_t x, uint16_t y) const;

    void resize(uint16_t newWidth, uint16_t newHeight);
    void clear();

    const std::string &name() const { return _name; }
    void setName(const std::string &n) { _name = n; }

private:
    uint16_t _width = 10;
    uint16_t _height = 10;
    MapType _mapType = MapType::WORLD;
    std::string _name = "nuevo_mapa";
    std::vector<Tile> _tiles;
};
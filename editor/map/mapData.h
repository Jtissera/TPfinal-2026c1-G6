#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "tile.h"

// Dimensiones máximas permitidas
static constexpr uint16_t MAP_MAX_WIDTH  = 200;
static constexpr uint16_t MAP_MAX_HEIGHT = 200;
static constexpr uint16_t MAP_MIN_SIZE   = 5;

class MapData {
public:
    MapData();
    MapData(uint16_t width, uint16_t height);

    uint16_t width()  const { return _width;  }
    uint16_t height() const { return _height; }

    // Acceso a tiles
    Tile&       at(uint16_t x, uint16_t y);
    const Tile& at(uint16_t x, uint16_t y) const;

    bool inBounds(uint16_t x, uint16_t y) const;

    // Redimensionar (rellena con tile por defecto)
    void resize(uint16_t newWidth, uint16_t newHeight);

    // Limpiar todo el mapa
    void clear();

    const std::string& name() const { return _name; }
    void setName(const std::string& n) { _name = n; }

private:
    uint16_t _width  = 10;
    uint16_t _height = 10;
    std::string _name = "nuevo_mapa";
    std::vector<Tile> _tiles;
};

#include "mapData.h"
#include <stdexcept>

MapData::MapData() { resize(_width, _height); }

MapData::MapData(uint16_t width, uint16_t height)
    : _width(width), _height(height) {
  resize(_width, _height);
}

bool MapData::inBounds(uint16_t x, uint16_t y) const {
  return x < _width && y < _height;
}

Tile &MapData::at(uint16_t x, uint16_t y) {
  if (!inBounds(x, y))
    throw std::out_of_range("Tile fuera del mapa");
  return _tiles[y * _width + x];
}

const Tile &MapData::at(uint16_t x, uint16_t y) const {
  if (!inBounds(x, y))
    throw std::out_of_range("Tile fuera del mapa");
  return _tiles[y * _width + x];
}

void MapData::resize(uint16_t newWidth, uint16_t newHeight) {
  _width = newWidth;
  _height = newHeight;
  _tiles.assign(_width * _height, Tile{});
}

void MapData::clear() { _tiles.assign(_width * _height, Tile{}); }

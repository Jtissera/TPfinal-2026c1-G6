#include "Map.h"
#include "ECS/Components.h"
#include "../Game.h"
#include "../../editor/map/mapSerializer.h"

Map::Map(Manager& manager, const std::string& textID, int mapScale, int tileSize)
    : manager(manager), textID(textID), mapScale(mapScale), tileSize(tileSize) {
    scaledSize = mapScale * tileSize;
}

static const char* tileTypeToTexture(TileType t) {
    switch(t) {
        case TileType::GRASS:            return "tile_grass";
        case TileType::WATER:            return "tile_water";
        case TileType::FLOOR:            return "tile_floor";
        case TileType::WALL:             return "tile_floor";
        case TileType::DOOR:             return "tile_floor";
        case TileType::DUNGEON_ENTRANCE: return "tile_floor";
        default:                         return "tile_grass";
    }
}

void Map::LoadMap(const std::string& path) {
    MapData mapData = MapSerializer::load(path);

    for (int y = 0; y < mapData.height(); y++) {
        for (int x = 0; x < mapData.width(); x++) {
            const Tile& t = mapData.at(x, y);
            const char* texId = tileTypeToTexture(t.type);
            AddTile(texId, x * scaledSize, y * scaledSize);
        }
    }
}

void Map::AddTile(const char* texId, int xpos, int ypos) {
    auto& tile = manager.addEntity();
    tile.addComponent<TileComponent>(0, 0, xpos, ypos, tileSize, mapScale, texId);
    tile.addGroup(Game::groupMap);
}
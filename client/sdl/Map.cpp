
#include "Map.h"
#include "ECS/Components.h"
#include "../Game.h"
#include <fstream>

Map::Map(Manager& manager, const std::string& textID, int mapScale, int tileSize)
    : manager(manager), textID(textID), mapScale(mapScale), tileSize(tileSize) {
    scaledSize = mapScale * tileSize;
}

void Map::LoadMap(const std::string& path, int sizeX, int sizeY) {
    std::fstream mapFile(path);
    int tileIndex;

    // Primera pasada: tiles visuales
    for (int y = 0; y < sizeY; y++) {
        for (int x = 0; x < sizeX; x++) {
            mapFile >> tileIndex;
            mapFile.ignore();
            int srcX = (tileIndex % 10) * tileSize;
            int srcY = (tileIndex / 10) * tileSize;
            AddTile(srcX, srcY, x * scaledSize, y * scaledSize);
        }
    }

    // Segunda pasada: colisiones
    for (int y = 0; y < sizeY; y++) {
        for (int x = 0; x < sizeX; x++) {
            mapFile >> tileIndex;
            mapFile.ignore();
            if (tileIndex == 1) {
                auto& tcol = manager.addEntity();
                tcol.addComponent<ColliderComponent>("terrain",
                    x * scaledSize, y * scaledSize, scaledSize);
                tcol.addGroup(Game::groupColliders);
            }
        }
    }

    mapFile.close();
}

void Map::AddTile(int srcX, int srcY, int xpos, int ypos) {
    auto& tile = manager.addEntity();
    tile.addComponent<TileComponent>(srcX, srcY, xpos, ypos, tileSize, mapScale, textID);
    tile.addGroup(Game::groupMap);
}


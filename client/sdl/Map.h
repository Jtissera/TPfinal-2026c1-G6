
#ifndef PRUEBA_SDL_MAP_H
#define PRUEBA_SDL_MAP_H
#include <string>

#include "AssetManager.h"
#include "ECS/ECS.h"
#include "editor/map/tile.h"

class Map
{
public:
    Map(Manager &manager, AssetManager &assets, const std::string &textID, int mapScale, int tileSize);
    ~Map() = default;

    void LoadMap(const std::string &path);

private:
    std::string GetRandomTextureForType(TileType type);
    void AddTile(const std::string &texId, int x, int y, TileType type);

    Manager &manager;
    AssetManager &assets;
    std::string textID;
    int mapScale;
    int tileSize;
    int scaledSize;
};

#endif // PRUEBA_SDL_MAP_H

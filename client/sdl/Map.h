
#ifndef PRUEBA_SDL_MAP_H
#define PRUEBA_SDL_MAP_H
#include <string>

#include "AssetManager.h"
#include "ECS/ECS.h"
#include "editor/map/tile.h"

class Map {
public:
    Map(Manager& manager,AssetManager& assets ,const std::string& textID, int mapScale, int tileSize);
    ~Map() = default;

    void LoadMap(const std::string& path);

private:
    const char* tileTypeToTexture(TileType type) const;
    void AddTile(const char* texId, int xpos, int ypos);

    Manager&    manager;
    AssetManager& assets;
    std::string textID;
    int         mapScale;
    int         tileSize;
    int         scaledSize;
};

#endif //PRUEBA_SDL_MAP_H

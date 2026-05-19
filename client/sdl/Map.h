
#ifndef PRUEBA_SDL_MAP_H
#define PRUEBA_SDL_MAP_H
#include <string>
#include "ECS/ECS.h"

class Map {
public:
    Map(Manager& manager, const std::string& textID, int mapScale, int tileSize);
    ~Map() = default;

    void LoadMap(const std::string& path, int sizeX, int sizeY);

private:
    void AddTile(int srcX, int srcY, int xpos, int ypos);

    Manager&    manager;
    std::string textID;
    int         mapScale;
    int         tileSize;
    int         scaledSize;
};

#endif //PRUEBA_SDL_MAP_H

#ifndef PRUEBA_SDL_MAP_H
#define PRUEBA_SDL_MAP_H
#include <string>
#include <vector>

#include "AssetManager.h"
#include "ECS/ECS.h"
#include "editor/map/tile.h"
#include <functional>

struct TileEntry
{
  SDL_Texture *texture = nullptr;
  SDL_Rect srcRect = {};
  SDL_Rect destRect = {};
  bool isTop = false;
  int groundY = 0;
};

class Map
{
public:
  Map(Manager &manager, AssetManager &assets, const std::string &textID,
      int mapScale, int tileSize);
  ~Map() = default;

  void LoadMap(const std::string &path);

  void renderLayer(SDL_Renderer *renderer, const SDL_Rect &camera,
                   const SDL_Rect &viewport, bool top) const;

  void forEachVisibleTopTile(const SDL_Rect &camera, const SDL_Rect &viewport,
                             const std::function<void(const TileEntry &)> &callback) const;

  int mapWidth() const { return width; }
  int mapHeight() const { return height; }

private:
  std::string GetRandomTextureForType(TileType type);
  void AddTile(const std::string &texId, int x, int y, TileType type);

  Manager &manager;
  AssetManager &assets;
  std::string textID;
  int mapScale;
  int tileSize;
  int scaledSize;

  int width = 0;
  int height = 0;

  std::vector<TileEntry> tiles;
};

#endif // PRUEBA_SDL_MAP_H

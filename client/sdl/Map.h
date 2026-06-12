#ifndef PRUEBA_SDL_MAP_H
#define PRUEBA_SDL_MAP_H
#include <string>
#include <vector>

#include "AssetManager.h"
#include "ECS/ECS.h"
#include "editor/map/tile.h"

struct TileEntry {
  SDL_Texture *texture = nullptr;
  SDL_Rect srcRect  = {};
  SDL_Rect destRect = {};  // posición en world-space (sin camara)
  bool isTop = false;      // true → groupMapTop
};

class Map {
public:
  Map(Manager &manager, AssetManager &assets, const std::string &textID,
      int mapScale, int tileSize);
  ~Map() = default;

  void LoadMap(const std::string &path);

  // Solo itera los tiles visibles en el viewport.
  // Evita iterar los 10.000 tiles del mapa vía ECS cada frame.
  void renderLayer(SDL_Renderer *renderer, const SDL_Rect &camera,
                   const SDL_Rect &viewport, bool top) const;

  int mapWidth()  const { return width; }
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

  int width  = 0;
  int height = 0;

  std::vector<TileEntry> tiles;
};

#endif // PRUEBA_SDL_MAP_H

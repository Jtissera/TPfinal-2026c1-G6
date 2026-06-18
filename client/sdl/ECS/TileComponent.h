
#ifndef PRUEBA_SDL_TILECOMPONENT_H
#define PRUEBA_SDL_TILECOMPONENT_H
#include "ECS.h"
#include "SDL2/SDL.h"
#include "Vector2D.h"
#include "client/sdl/AssetManager.h"
#include <string>

class TileComponent : public Component
{
public:
  SDL_Texture *texture = nullptr;
  SDL_Rect srcRect{};
  SDL_Rect destRect{};
  Vector2D position;
  int getWorldFootprintY() const;

  TileComponent() = default;
  TileComponent(AssetManager &assets, int srcX, int srcY, int xpos, int ypos,
                int srcW, int srcH, int tscale, const std::string &id);
  ~TileComponent();

  void update(UpdateContext &context) override;
  void draw(RenderContext &context) override;

  const SDL_Rect &getDestRect() const { return destRect; }
  const SDL_Rect &getSrcRect() const { return srcRect; }

private:
  int mapScale = 3;
};

#endif // PRUEBA_SDL_TILECOMPONENT_H

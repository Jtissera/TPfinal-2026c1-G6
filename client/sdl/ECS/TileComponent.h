
#ifndef PRUEBA_SDL_TILECOMPONENT_H
#define PRUEBA_SDL_TILECOMPONENT_H
#include <string>
#include "SDL2/SDL.h"
#include "ECS.h"
#include "Vector2D.h"

class TileComponent : public Component {
public:
    SDL_Texture* texture = nullptr;
    SDL_Rect srcRect{};
    SDL_Rect destRect{};
    Vector2D position;

    TileComponent() = default;
    TileComponent(int srcX, int srcY, int xpos, int ypos,
                  int tsize, int tscale, const std::string& id);
    ~TileComponent();

    void update() override;
    void draw() override;
};

#endif //PRUEBA_SDL_TILECOMPONENT_H

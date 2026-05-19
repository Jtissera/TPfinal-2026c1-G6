
#ifndef PRUEBA_SDL_COLIDERCOMPONENT_H
#define PRUEBA_SDL_COLIDERCOMPONENT_H
#include <string>
#include "SDL2/SDL.h"
#include "ECS.h"
#include "TransformComponent.h"

class ColliderComponent : public Component {
public:
    SDL_Rect collider{};
    std::string tag;

    // Para entidades con transform (jugador, enemigos)
    explicit ColliderComponent(const std::string& t);

    // Para colisores de terreno (posición fija)
    ColliderComponent(const std::string& t, int xpos, int ypos, int size);

    void init() override;
    void update() override;
    void draw() override;

private:
    SDL_Texture* tex = nullptr;
    SDL_Rect srcR{};
    SDL_Rect destR{};
    TransformComponent* transform = nullptr;
};


#endif //PRUEBA_SDL_COLIDERCOMPONENT_H

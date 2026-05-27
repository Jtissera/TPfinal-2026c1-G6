
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

    explicit ColliderComponent(const std::string& t);
    ColliderComponent(const std::string& t, int xpos, int ypos, int size);

    void init() override;
    void update(UpdateContext& context) override;
    void draw(RenderContext& context) override;

private:
    SDL_Rect destR{};
    TransformComponent* transform = nullptr;
};

#endif // PRUEBA_SDL_COLIDERCOMPONENT_H


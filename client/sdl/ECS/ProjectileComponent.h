
#ifndef PRUEBA_SDL_PROJECTILE_H
#define PRUEBA_SDL_PROJECTILE_H

#include "ECS.h"
#include "Components.h"
#include "Vector2D.h"
#include "TransformComponent.h"

class ProjectileComponent : public Component {
public:
    ProjectileComponent(int range, int speed, Vector2D vel);

    void init() override;
    void update() override;

private:
    TransformComponent* transform = nullptr;
    int range;
    int speed;
    int distance = 0;
    Vector2D velocity;
};







#endif //PRUEBA_SDL_PROJECTILE_H

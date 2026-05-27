
#ifndef PRUEBA_SDL_POSITIONCOMPONENT_H
#define PRUEBA_SDL_POSITIONCOMPONENT_H

#include  "Vector2D.h"
#include "ECS.h"
#include "Vector2D.h"
#include "ECS.h"

class TransformComponent : public Component {
public:
    Vector2D position;
    Vector2D velocity;
    int speed = 3;
    int height = 32;
    int width = 32;
    int scale = 1;

    TransformComponent();
    explicit TransformComponent(int sc);
    TransformComponent(float x, float y);
    TransformComponent(float x, float y, int h, int w, int sc);

    void setPos(float x, float y);
    void update(UpdateContext& context) override;
    void init() override;
};

#endif //PRUEBA_SDL_POSITIONCOMPONENT_H

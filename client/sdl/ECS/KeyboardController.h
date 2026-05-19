
#ifndef PRUEBA_SDL_KEYBOARDCONTROLLER_H
#define PRUEBA_SDL_KEYBOARDCONTROLLER_H
#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"

class KeyboardController : public Component {
public:
    void init() override;
    void update() override;

private:
    TransformComponent* transform = nullptr;
    SpriteComponent*    sprite    = nullptr;
};




#endif //PRUEBA_SDL_KEYBOARDCONTROLLER_H

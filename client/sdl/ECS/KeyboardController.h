
#ifndef PRUEBA_SDL_KEYBOARDCONTROLLER_H
#define PRUEBA_SDL_KEYBOARDCONTROLLER_H
#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"
#include "../../../common/network/protocol/protocol.h"

class KeyboardController : public Component {
public:
    explicit KeyboardController(Protocol& protocol);
    void init() override;
    void update() override;

private:
    TransformComponent* transform = nullptr;
    SpriteComponent*    sprite    = nullptr;
    Protocol&           protocol;
};




#endif //PRUEBA_SDL_KEYBOARDCONTROLLER_H

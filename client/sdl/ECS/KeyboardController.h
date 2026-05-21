
#ifndef PRUEBA_SDL_KEYBOARDCONTROLLER_H
#define PRUEBA_SDL_KEYBOARDCONTROLLER_H
#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"
#include "../../../common/network/protocol/protocol.h"
#include "common/queue.h"

class KeyboardController : public Component {
public:
    explicit KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue);
    void init() override;
    void update() override;

private:
    TransformComponent* transform = nullptr;
    SpriteComponent*    sprite    = nullptr;
    Queue<std::shared_ptr<const Message>>& sendQueue;
    bool movingUp    = false;
    bool movingDown  = false;
    bool movingLeft  = false;
    bool movingRight = false;
};

#endif //PRUEBA_SDL_KEYBOARDCONTROLLER_H

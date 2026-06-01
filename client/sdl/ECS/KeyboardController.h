
#ifndef PRUEBA_SDL_KEYBOARDCONTROLLER_H
#define PRUEBA_SDL_KEYBOARDCONTROLLER_H
#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"
#include "../../../common/network/protocol/protocol.h"
#include "common/queue.h"
#include "common/dtos/gameTypes.h"


enum class FacingDirection {
    Down,
    Up,
    Left,
    Right
};
class KeyboardController : public Component {
public:
    explicit KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue);
    void init() override;
    void update(UpdateContext& context) override;

private:

    TransformComponent* transform = nullptr;
    SpriteComponent*    sprite    = nullptr;
    FacingDirection lastDirection = FacingDirection::Down;
    Queue<std::shared_ptr<const Message>>& sendQueue;
    Uint32 lastMoveSentAt = 0;
    Uint32 moveCooldownMs = 140;
    bool movingUp    = false;
    bool movingDown  = false;
    bool movingLeft  = false;
    bool movingRight = false;

    void sendMoveIfReady(UpdateContext& context, Direction direction);
};

#endif //PRUEBA_SDL_KEYBOARDCONTROLLER_H

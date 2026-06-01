#include "KeyboardController.h"
#include "../../Game.h"
#include "common/network/messages/client/movement/moveMessage.h"


KeyboardController::KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue)
    : sendQueue(sendQueue){}

void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
    transform->velocity.Zero();
    // Evita que TransformComponent empuje al jugador local entre updates del server.
    transform->speed = 0;
}

void KeyboardController::update(UpdateContext& context) {
    const Uint8* keys = context.keyboardState;

    // Evita movimiento local acumulado.
    // El cliente solo manda intención; no mueve directamente al jugador.
    transform->velocity.Zero();
    movingUp    = keys[SDL_SCANCODE_W];
    movingDown  = keys[SDL_SCANCODE_S];
    movingLeft  = keys[SDL_SCANCODE_A];
    movingRight = keys[SDL_SCANCODE_D];

    
    if (movingUp) {
        lastDirection = FacingDirection::Up;
        sendMoveIfReady(context, Direction::UP);
        sprite->Play("WalkUp");

    } else if (movingDown) {
        lastDirection = FacingDirection::Down;
        sendMoveIfReady(context, Direction::DOWN);
        sprite->Play("WalkDown");

    } else if (movingLeft) {
        lastDirection = FacingDirection::Left;
        sendMoveIfReady(context, Direction::LEFT);
        sprite->Play("WalkLeft");

    } else if (movingRight) {
        lastDirection = FacingDirection::Right;
        sendMoveIfReady(context, Direction::RIGHT);
        sprite->Play("WalkRight");

    } else {
        switch (lastDirection) {
            case FacingDirection::Up:
                sprite->Play("IdleUp");
                break;
            case FacingDirection::Down:
                sprite->Play("IdleDown");
                break;
            case FacingDirection::Left:
                sprite->Play("IdleLeft");
                break;
            case FacingDirection::Right:
                sprite->Play("IdleRight");
                break;
        }
    }
}

void KeyboardController::sendMoveIfReady(UpdateContext& context, Direction direction) {
    Uint32 now = SDL_GetTicks();

    if (now - lastMoveSentAt < moveCooldownMs) {
        return;
    }

    context.sendQueue->push(
        std::make_shared<MoveMessage>(direction)
    );

    lastMoveSentAt = now;
}
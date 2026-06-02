#include "KeyboardController.h"
#include "../../Game.h"
#include "common/network/messages/client/movement/moveMessage.h"


KeyboardController::KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue)
    : sendQueue(sendQueue){}

void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
}

void KeyboardController::update(UpdateContext& context) {
    const Uint8* keys = context.keyboardState;

    // Leemos el estado actual del teclado.
    movingUp    = keys[SDL_SCANCODE_W];
    movingDown  = keys[SDL_SCANCODE_S];
    movingLeft  = keys[SDL_SCANCODE_A];
    movingRight = keys[SDL_SCANCODE_D];

    if (movingUp) {
        lastDirection = FacingDirection::Up;
        sendMoveIfReady(Direction::UP);
        sprite->Play("WalkUp");
        wasMoving = true;
        return;
    }

    if (movingDown) {
        lastDirection = FacingDirection::Down;

        sendMoveIfReady(Direction::DOWN);
        sprite->Play("WalkDown");

        wasMoving = true;
        return;
    }

    if (movingLeft) {
        lastDirection = FacingDirection::Left;

        sendMoveIfReady(Direction::LEFT);
        sprite->Play("WalkLeft");

        wasMoving = true;
        return;
    }

    if (movingRight) {
        lastDirection = FacingDirection::Right;

        sendMoveIfReady(Direction::RIGHT);
        sprite->Play("WalkRight");

        wasMoving = true;
        return;
    }

    // Si llegamos acá, no hay tecla de movimiento presionada.
    // Primero actualizamos la animación local a Idle.
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
    if (wasMoving) {
        const Direction stopDirection = toNetworkDirection(lastDirection);

        sendQueue.try_push(std::make_shared<const MoveMessage>(
            stopDirection,
            false
        ));

        std::cout << "[CLIENT INPUT] STOP enviado. direction="
                  << static_cast<int>(stopDirection)
                  << std::endl;

        wasMoving = false;
    }
}

void KeyboardController::sendMoveIfReady(Direction direction) {
    const Uint32 now = SDL_GetTicks();
    if (now - lastMoveSentAt < moveCooldownMs) {
        return;
    }

    lastMoveSentAt = now;

    // moving=true porque este mensaje representa una caminata.
    sendQueue.try_push(std::make_shared<const MoveMessage>(direction, true));
}

Direction KeyboardController::toNetworkDirection(FacingDirection facing) const {
    switch (facing) {
        case FacingDirection::Up:
            return Direction::UP;

        case FacingDirection::Down:
            return Direction::DOWN;

        case FacingDirection::Left:
            return Direction::LEFT;

        case FacingDirection::Right:
            return Direction::RIGHT;
    }
    return Direction::DOWN;
}

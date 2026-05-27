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
    // Lee el estado actual del teclado.
    // Esto permite saber si una tecla está mantenida presionada.
    const Uint8* keys = context.keyboardState;

    // Actualiza los flags de movimiento según WASD.
    movingUp    = keys[SDL_SCANCODE_W];
    movingDown  = keys[SDL_SCANCODE_S];
    movingLeft  = keys[SDL_SCANCODE_A];
    movingRight = keys[SDL_SCANCODE_D];

    // Por ahora damos prioridad a una sola dirección.
    // Esto evita que se pisen animaciones si se aprietan dos teclas a la vez.
    if (movingUp) {
        // Guarda hacia dónde quedó mirando el personaje.
        lastDirection = FacingDirection::Up;

        // Envía el movimiento al servidor.
        context.sendQueue->push(std::make_shared<MoveMessage>(Direction::UP));

        // Reproduce la animación de caminar hacia arriba.
        sprite->Play("WalkUp");


    } else if (movingDown) {
        // Guarda dirección visual.
        lastDirection = FacingDirection::Down;

        // Envía movimiento al servidor.
        context.sendQueue->push(std::make_shared<MoveMessage>(Direction::DOWN));

        // Reproduce caminar hacia abajo.
        sprite->Play("WalkDown");

    } else if (movingLeft) {
        // Guarda dirección visual.
        lastDirection = FacingDirection::Left;

        // Envía movimiento al servidor.
        context.sendQueue->push(std::make_shared<MoveMessage>(Direction::LEFT));

        // Reproduce caminar hacia izquierda.
        sprite->Play("WalkLeft");


    } else if (movingRight) {
        // Guarda dirección visual.
        lastDirection = FacingDirection::Right;

        // Envía movimiento al servidor.
        context.sendQueue->push(std::make_shared<MoveMessage>(Direction::RIGHT));

        // Reproduce caminar hacia derecha.
        sprite->Play("WalkRight");

    } else {
        // Si no se mueve, queda mirando hacia la última dirección usada.
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
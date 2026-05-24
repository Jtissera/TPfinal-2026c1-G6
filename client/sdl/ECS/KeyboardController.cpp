#include "KeyboardController.h"
#include "../../Game.h"
#include "common/network/messages/client/movement/moveMessage.h"


KeyboardController::KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue)
    : sendQueue(sendQueue){}

void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
}

void KeyboardController::update() {
    SDL_Event& event = Game::event;

    // Detecta cuándo una tecla empieza a presionarse.
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
                movingUp = true;
                break;

            case SDLK_s:
                movingDown = true;
                break;

            case SDLK_a:
                movingLeft = true;
                break;

            case SDLK_d:
                movingRight = true;
                break;
        }
    }

    // Detecta cuándo una tecla deja de presionarse.
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
                movingUp = false;
                break;

            case SDLK_s:
                movingDown = false;
                break;

            case SDLK_a:
                movingLeft = false;
                break;

            case SDLK_d:
                movingRight = false;
                break;
        }
    }

    if (movingUp) {
        // Guarda la última dirección visual.
        lastDirection = FacingDirection::Up;

        // Envia movimiento al servidor.
        sendQueue.push(std::make_shared<MoveMessage>(Direction::UP));

        // Reproduce la animación de caminar hacia arriba.
        sprite->Play("WalkUp");

        // No se usa flip porque el spritesheet ya tiene filas por dirección.
        sprite->spriteFlip = SDL_FLIP_NONE;

    } else if (movingDown) {
        lastDirection = FacingDirection::Down;

        sendQueue.push(std::make_shared<MoveMessage>(Direction::DOWN));

        sprite->Play("WalkDown");

        sprite->spriteFlip = SDL_FLIP_NONE;

    } else if (movingLeft) {
        lastDirection = FacingDirection::Left;

        sendQueue.push(std::make_shared<MoveMessage>(Direction::LEFT));

        sprite->Play("WalkLeft");

        sprite->spriteFlip = SDL_FLIP_NONE;

    } else if (movingRight) {
        lastDirection = FacingDirection::Right;

        sendQueue.push(std::make_shared<MoveMessage>(Direction::RIGHT));

        sprite->Play("WalkRight");

        sprite->spriteFlip = SDL_FLIP_NONE;

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

        sprite->spriteFlip = SDL_FLIP_NONE;
    }
}
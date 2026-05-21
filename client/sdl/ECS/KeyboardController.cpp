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
    // En update():
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        switch (event.key.keysym.sym) {
            case SDLK_w: movingUp    = true; break;
            case SDLK_s: movingDown  = true; break;
            case SDLK_a: movingLeft  = true; break;
            case SDLK_d: movingRight = true; break;
        }
    }
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w: movingUp    = false; break;
            case SDLK_s: movingDown  = false; break;
            case SDLK_a: movingLeft  = false; break;
            case SDLK_d: movingRight = false; break;
        }
    }

    // Mandar el mensaje según qué tecla está activa
    if (movingUp)    sendQueue.push(std::make_shared<MoveMessage>(Direction::UP));
    if (movingDown)  sendQueue.push(std::make_shared<MoveMessage>(Direction::DOWN));
    if (movingLeft)  sendQueue.push(std::make_shared<MoveMessage>(Direction::LEFT));
    if (movingRight) sendQueue.push(std::make_shared<MoveMessage>(Direction::RIGHT));
    
    // Animación y flip según estado
    if (movingUp || movingDown || movingLeft || movingRight) {
        sprite->Play("Walk");
    } else {
        sprite->Play("Idle");
    }

    // Flip según dirección horizontal
    if (movingLeft) {
        sprite->spriteFlip = SDL_FLIP_HORIZONTAL;
    } else {
        sprite->spriteFlip = SDL_FLIP_NONE;
    }
}
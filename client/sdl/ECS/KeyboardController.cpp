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
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    movingUp    = keys[SDL_SCANCODE_W];
    movingDown  = keys[SDL_SCANCODE_S];
    movingLeft  = keys[SDL_SCANCODE_A];
    movingRight = keys[SDL_SCANCODE_D];

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
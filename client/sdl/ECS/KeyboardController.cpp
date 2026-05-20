#include "KeyboardController.h"
#include "../Game.h"
#include "common/network/messages/client/movement/moveMessage.h"


KeyboardController::KeyboardController(Protocol& protocol)
    : protocol(protocol) {}
void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
}

void KeyboardController::update() {
    SDL_Event& event = Game::event;

    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
                protocol.send(MoveMessage(Direction::UP));
                sprite->Play("Walk");
                break;
            case SDLK_s:
                protocol.send(MoveMessage(Direction::DOWN));
                sprite->Play("Walk");
                break;
            case SDLK_a:
                protocol.send(MoveMessage(Direction::LEFT));
                sprite->Play("Walk");
                sprite->spriteFlip = SDL_FLIP_HORIZONTAL;
                break;
            case SDLK_d:
                protocol.send(MoveMessage(Direction::RIGHT));
                sprite->Play("Walk");
                break;
            default:
                break;
        }
    }

    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
            case SDLK_s:
            case SDLK_a:
                sprite->spriteFlip = SDL_FLIP_NONE;
                sprite->Play("Idle");
                break;
            case SDLK_d:
                sprite->Play("Idle");
                break;
            case SDLK_ESCAPE:
                Game::isRunning = false;
                break;
            default:
                break;
        }
    }
}
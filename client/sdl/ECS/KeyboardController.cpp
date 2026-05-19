#include "KeyboardController.h"
#include "../Game.h"

void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
}

void KeyboardController::update() {
    SDL_Event& event = Game::event;

    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
                transform->velocity.y = -1;
                sprite->Play("Walk");
                break;
            case SDLK_s:
                transform->velocity.y = 1;
                sprite->Play("Walk");
                break;
            case SDLK_a:
                transform->velocity.x = -1;
                sprite->Play("Walk");
                sprite->spriteFlip = SDL_FLIP_HORIZONTAL;
                break;
            case SDLK_d:
                transform->velocity.x = 1;
                sprite->Play("Walk");
                break;
            default:
                break;
        }
    }

    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_w:
                transform->velocity.y = 0;
                sprite->Play("Idle");
                break;
            case SDLK_s:
                transform->velocity.y = 0;
                sprite->Play("Idle");
                break;
            case SDLK_a:
                transform->velocity.x = 0;
                sprite->Play("Idle");
                sprite->spriteFlip = SDL_FLIP_NONE;
                break;
            case SDLK_d:
                transform->velocity.x = 0;
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
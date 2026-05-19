
#include "SpriteComponent.h"
#include "../TextureManager.h"
#include "../Game.h"

SpriteComponent::SpriteComponent(const char* path) {
    setText(path);
}

SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated) {
    animated = isAnimated;
    animations.emplace("Idle", Animation(0, 3, 100));
    animations.emplace("Walk", Animation(1, 6, 100));
    Play("Idle");
    setText(id);
}

void SpriteComponent::setText(const std::string& id) {
    texture = Game::assets->GetTexture(id);
}

void SpriteComponent::Play(const char* animName) {
    frames         = animations[animName].frames;
    animationIndex = animations[animName].index;
    speed          = animations[animName].speed;
}

void SpriteComponent::init() {
    transform = &entity->getComponent<TransformComponent>();
    srcRect.x = srcRect.y = 0;
    srcRect.w = transform->width;
    srcRect.h = transform->height;
}

void SpriteComponent::update() {
    if (animated) {
        srcRect.x = srcRect.w * static_cast<int>((SDL_GetTicks() / speed) % frames);
    }
    srcRect.y = animationIndex * transform->height;

    destRect.x = static_cast<int>(transform->position.x) - Game::camera.x;
    destRect.y = static_cast<int>(transform->position.y) - Game::camera.y;
    destRect.w = transform->width  * transform->scale;
    destRect.h = transform->height * transform->scale;
}

void SpriteComponent::draw() {
    TextureManager::Draw(texture, srcRect, destRect, spriteFlip);
}
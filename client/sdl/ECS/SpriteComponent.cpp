
#include "SpriteComponent.h"
#include "../TextureManager.h"
#include "../Game.h"

SpriteComponent::SpriteComponent(const char* path) {
    setText(path);
}

SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated) {
    animated = isAnimated;
    animations.emplace("Idle", Animation(0, 6, 200));
    animations.emplace("Walk", Animation(0, 6, 100));
    Play("Idle");
    setText(id);
}

void SpriteComponent::setText(const std::string& id) {
    texture = Game::assets->GetTexture(id);
}

void SpriteComponent::Play(const char* animName) {
    std::string name(animName);
    if (animations.count(name) == 0) return;
    frames         = animations[name].frames;
    animationIndex = animations[name].index;
    speed          = animations[name].speed;
}

void SpriteComponent::init() {
    transform = &entity->getComponent<TransformComponent>();
    srcRect.x = srcRect.y = 0;
    srcRect.w = transform->width;
    srcRect.h = transform->height;
}

void SpriteComponent::update() {
    if (animated && frames > 0) {  // ← agregar el frames > 0
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

#include "SpriteComponent.h"
#include "../TextureManager.h"
#include "../../Game.h"

SpriteComponent::SpriteComponent(const char* path) {
    setText(path);
}

SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated) {
    animated = isAnimated;
    // index = fila (0=sur, 1=oeste, 2=este, 3=norte)
    // frames = cantidad de frames en esa fila
    // speed  = ms por frame
    animations.emplace("Idle",   Animation(0, 4,  150));
    animations.emplace("Walk",   Animation(0, 6,  100));
    //animations.emplace("Attack", Animation(0, 12, 80));
    //animations.emplace("Hurt",   Animation(0, 4,  100));
    Play("Idle");
    setText(id);
}

SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated,
                                 std::map<std::string, Animation> anims) {
    animated = isAnimated;
    animations = anims;
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
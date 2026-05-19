#include "ColliderComponent.h"
#include "../TextureManager.h"
#include "../Game.h"

ColliderComponent::ColliderComponent(const std::string& t) {
    tag = t;
}

ColliderComponent::ColliderComponent(const std::string& t, int xpos, int ypos, int size) {
    tag = t;
    collider.x = xpos;
    collider.y = ypos;
    collider.w = size;
    collider.h = size;
}

void ColliderComponent::init() {
    if (!entity->hasComponent<TransformComponent>()) {
        entity->addComponent<TransformComponent>();
    }
    transform = &entity->getComponent<TransformComponent>();
    tex = TextureManager::loadTexture("assets/sprites/MapAssets/ColTex.png");
    srcR = {0, 0, 32, 32};
    destR = {collider.x, collider.y, collider.w, collider.h};
}

void ColliderComponent::update() {
    if (tag != "terrain") {
        collider.x = static_cast<int>(transform->position.x);
        collider.y = static_cast<int>(transform->position.y);
        collider.w = transform->width  * transform->scale;
        collider.h = transform->height * transform->scale;
    }
    destR.x = collider.x - Game::camera.x;
    destR.y = collider.y - Game::camera.y;
}

void ColliderComponent::draw() {
    SDL_SetRenderDrawColor(Game::renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(Game::renderer, &destR);
}
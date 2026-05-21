
#include "ProjectileComponent.h"
#include "../../Game.h"
#include <iostream>

ProjectileComponent::ProjectileComponent(int range, int speed, Vector2D vel)
    : range(range), speed(speed), velocity(vel) {}

void ProjectileComponent::init() {
    transform = &entity->getComponent<TransformComponent>();
    transform->velocity = velocity;
}

void ProjectileComponent::update() {
    distance += speed;

    if (distance > range) {
        std::cout << "Out of range" << std::endl;
        entity->destroy();
        return;
    }

    bool outOfBounds =
        transform->position.x > Game::camera.x + Game::camera.w ||
        transform->position.x < Game::camera.x ||
        transform->position.y > Game::camera.y + Game::camera.h ||
        transform->position.y < Game::camera.y;

    if (outOfBounds) {
        std::cout << "Out of bounds" << std::endl;
        entity->destroy();
    }
}


#include "ProjectileComponent.h"
#include "../../Game.h"
#include <iostream>

ProjectileComponent::ProjectileComponent(int range, int speed, Vector2D vel)
    : range(range), speed(speed), velocity(vel) {}

void ProjectileComponent::init() {
    transform = &entity->getComponent<TransformComponent>();
    transform->velocity = velocity;
}

void ProjectileComponent::update(UpdateContext& context){
    transform->position.x += transform->velocity.x * speed;
    transform->position.y += transform->velocity.y * speed;
    distance += speed;

    if (distance > range) {
        std::cout << "Out of range" << std::endl;
        entity->destroy();
        return;
    }

    bool outOfBounds =
        transform->position.x > context.camera.x + context.camera.w ||
        transform->position.x < context.camera.x ||
        transform->position.y > context.camera.y + context.camera.h ||
        transform->position.y < context.camera.y;

    if (outOfBounds) {
        std::cout << "Out of bounds" << std::endl;
        entity->destroy();
    }
}

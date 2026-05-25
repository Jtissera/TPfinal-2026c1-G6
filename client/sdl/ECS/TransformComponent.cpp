#include "TransformComponent.h"

TransformComponent::TransformComponent() {
    position.Zero();
}

TransformComponent::TransformComponent(int sc) {
    position.x = 576;
    position.y = 676;
    scale = sc;
}

TransformComponent::TransformComponent(float x, float y) {
    position.x = x;
    position.y = y;
}

TransformComponent::TransformComponent(float x, float y, int h, int w, int sc) {
    position.x = x;
    position.y = y;
    height = h;
    width = w;
    scale = sc;
}

void TransformComponent::setPos(float x, float y) {
    position.x = x;
    position.y = y;
}

void TransformComponent::update() {
    //position.x += velocity.x * speed;
    //position.y += velocity.y * speed;
}

void TransformComponent::init() {
    velocity.Zero();
}



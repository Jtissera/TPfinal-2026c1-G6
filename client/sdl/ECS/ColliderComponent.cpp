#include "ColliderComponent.h"
#include "../TextureManager.h"
#include "../../Game.h"

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
    if (entity->hasComponent<TransformComponent>()) {
        transform = &entity->getComponent<TransformComponent>();
    }
    destR = {
        collider.x,
        collider.y,
        collider.w,
        collider.h
    };
}

void ColliderComponent::update(UpdateContext& context) {
    // El contexto no se usa para actualizar el collider.
    // La cámara solo se usa al dibujar.
    (void)context;

    if (tag != "terrain" && transform != nullptr) {
        collider.x = static_cast<int>(transform->position.x);
        collider.y = static_cast<int>(transform->position.y);
        collider.w = transform->width * transform->scale;
        collider.h = transform->height * transform->scale;
    }
}

void ColliderComponent::draw(RenderContext& context) {
    if (context.renderer == nullptr) {
        return;
    }

    // Convertimos coordenadas de mundo a pantalla.
    destR.x = collider.x - context.camera.x;
    destR.y = collider.y - context.camera.y + context.mapOffsetY;
    destR.w = collider.w;
    destR.h = collider.h;

    // Debug visual del collider.
    SDL_SetRenderDrawColor(context.renderer, 255, 255, 0, 255);
    SDL_RenderDrawRect(context.renderer, &destR);

    // Restauramos color base.
    SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
}
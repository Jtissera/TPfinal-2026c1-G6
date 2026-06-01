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

    targetPosition.x = x;
    targetPosition.y = y;
}

void TransformComponent::update(UpdateContext& context) {
    (void)context;

    if (interpolateToTarget) {
        // Calculamos la diferencia entre donde estoy dibujado
        // y donde el servidor dice que debería estar.
        float dx = targetPosition.x - position.x;
        float dy = targetPosition.y - position.y;

        // Distancia al objetivo.
        float distance = std::sqrt(dx * dx + dy * dy);

        // Si estoy muy cerca, clavo la posición exacta.
        // Esto evita vibraciones por decimales.
        if (distance < 1.0f) {
            position.x = targetPosition.x;
            position.y = targetPosition.y;
            return;
        }

        // Como tu loop está limitado a 30 FPS, usamos un delta aproximado.
        // Mejor sería pasar deltaTime real por UpdateContext.
        float deltaTime = 1.0f / 30.0f;

        // Cantidad máxima que puedo avanzar este frame.
        float step = interpolationSpeed * deltaTime;

        // Si el paso supera la distancia, llego directo al target.
        if (step >= distance) {
            position.x = targetPosition.x;
            position.y = targetPosition.y;
            return;
        }

        // Normalizamos la dirección y avanzamos suavemente.
        position.x += (dx / distance) * step;
        position.y += (dy / distance) * step;

        return;
    }

    // Movimiento tradicional para entidades que no usan interpolación.
    position.x += velocity.x * speed;
    position.y += velocity.y * speed;
}

void TransformComponent::init() {
    velocity.Zero();
}


void TransformComponent::setTargetPos(float x, float y) {
    // Guarda la posición real enviada por el servidor.
    // No modifica directamente la posición visual.
    targetPosition.x = x;
    targetPosition.y = y;
}


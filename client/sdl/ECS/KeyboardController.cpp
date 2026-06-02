#include "KeyboardController.h"
#include "common/network/messages/client/movement/moveMessage.h"

KeyboardController::KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue)
    : sendQueue(sendQueue) {}

void KeyboardController::init() {
    transform = &entity->getComponent<TransformComponent>();
    sprite    = &entity->getComponent<SpriteComponent>();
}

void KeyboardController::update(UpdateContext& ctx) {
    const Uint8* keys = ctx.keyboardState;
    Uint32 now = SDL_GetTicks();

    bool wantUp    = keys[SDL_SCANCODE_W];
    bool wantDown  = keys[SDL_SCANCODE_S];
    bool wantLeft  = keys[SDL_SCANCODE_A];
    bool wantRight = keys[SDL_SCANCODE_D];

    holdingKey = wantUp || wantDown || wantLeft || wantRight;

    // Actualizar dirección siempre — Game la lee para la animación
    if      (wantUp)    lastDirection = FacingDirection::Up;
    else if (wantDown)  lastDirection = FacingDirection::Down;
    else if (wantLeft)  lastDirection = FacingDirection::Left;
    else if (wantRight) lastDirection = FacingDirection::Right;

    // Idle por defecto — Game overridea con Walk cuando hay movimiento activo
    switch (lastDirection) {
        case FacingDirection::Up:    sprite->Play("IdleUp");    break;
        case FacingDirection::Down:  sprite->Play("IdleDown");  break;
        case FacingDirection::Left:  sprite->Play("IdleLeft");  break;
        case FacingDirection::Right: sprite->Play("IdleRight"); break;
    }

    // Envío de mensajes al servidor
    if (holdingKey) {
        if (now - lastSendMs >= SEND_INTERVAL_MS) {
            lastSendMs = now;
            if      (wantUp)    ctx.sendQueue->push(std::make_shared<MoveMessage>(Direction::UP));
            else if (wantDown)  ctx.sendQueue->push(std::make_shared<MoveMessage>(Direction::DOWN));
            else if (wantLeft)  ctx.sendQueue->push(std::make_shared<MoveMessage>(Direction::LEFT));
            else if (wantRight) ctx.sendQueue->push(std::make_shared<MoveMessage>(Direction::RIGHT));
        }
    } else {
        lastSendMs = 0;  // reset para que el próximo movimiento arranque inmediato
    }
}
#include "RemotePlayer.h"

#include "common/dtos/gameTypes.h"

RemotePlayer::RemotePlayer(uint32_t id, Entity* entity)
    : id(id),
      entity(entity) {
}

uint32_t RemotePlayer::getId() const {
    return id;
}

Entity* RemotePlayer::getEntity() const {
    return entity;
}

void RemotePlayer::setPositionAndAnimation(float x,float y,Direction direction,bool moving) {
    // Si por algún error no hay entidad, no intentamos acceder a componentes.
    if (entity == nullptr) {
        return;
    }
    std::cout << "[REMOTE SYNC] id="
          << id
          << " pos=(" << x << ", " << y << ")"
          << std::endl;

    // Obtenemos el TransformComponent de la entidad remota.
    auto& transform = entity->getComponent<TransformComponent>();

    // Aplicamos posición enviada por el servidor.
    transform.position.x = x;
    transform.position.y = y;

    if (!entity->hasComponent<SpriteComponent>()) {
        return;
    }

    auto& sprite = entity->getComponent<SpriteComponent>();

    if (moving) {
        switch (direction) {
            case Direction::UP:
                sprite.Play("WalkUp");
                break;

            case Direction::DOWN:
                sprite.Play("WalkDown");
                break;

            case Direction::LEFT:
                sprite.Play("WalkLeft");
                break;

            case Direction::RIGHT:
                sprite.Play("WalkRight");
                break;
            case Direction::NONE:
                sprite.Play("WalkDown");
                break;
        }


        return;
    }

    switch (direction) {
        case Direction::UP:
            sprite.Play("IdleUp");
            break;

        case Direction::DOWN:
            sprite.Play("IdleDown");
            break;

        case Direction::LEFT:
            sprite.Play("IdleLeft");
            break;

        case Direction::RIGHT:
            sprite.Play("IdleRight");
            break;
        case Direction::NONE:
            sprite.Play("WalkDown");
            break;
    }
}

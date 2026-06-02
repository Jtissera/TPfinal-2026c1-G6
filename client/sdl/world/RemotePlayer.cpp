#include "RemotePlayer.h"

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

void RemotePlayer::setPosition(float x, float y) {
    // Si por algún error no hay entidad, no intentamos acceder a componentes.
    if (entity == nullptr) {
        return;
    }

    // Obtenemos el TransformComponent de la entidad remota.
    auto& transform = entity->getComponent<TransformComponent>();

    // Aplicamos posición enviada por el servidor.
    transform.position.x = x;
    transform.position.y = y;
}

#include "ClientGameWorld.h"

#include <iostream>

#include "../ECS/Components.h"


ClientGameWorld::ClientGameWorld(uint32_t localPlayerId,Entity* localPlayer,AssetManager& assets)
    : localPlayerId(localPlayerId),
      localPlayer(localPlayer),
      assets(assets) {
}

bool ClientGameWorld::isLocalPlayer(uint32_t entityId) const {
    // Compara el id recibido por red contra el id local.
    return entityId == localPlayerId;
}

bool ClientGameWorld::hasRemotePlayer(uint32_t entityId) const {
    // Busca si el remoto ya existe en el mapa.
    return remotePlayers.find(entityId) != remotePlayers.end();
}

void ClientGameWorld::spawnRemotePlayer(const PlayerDto& remotePlayerDto) {
    // Tomamos el id del jugador remoto enviado por el servidor.
    const uint32_t entityId = static_cast<uint32_t>(remotePlayerDto.playerID);

    // Si el id corresponde al jugador local, no lo creamos como remoto.
    if (isLocalPlayer(entityId)) {
        std::cout << "[CLIENT_WORLD] Spawn ignorado para jugador local. id="
                  << entityId
                  << std::endl;
        return;
    }

    // Si ya existe, no duplicamos la entidad visual.
    if (hasRemotePlayer(entityId)) {
        std::cout << "[CLIENT_WORLD] RemotePlayer ya existe. id="
                  << entityId
                  << std::endl;
        return;
    }

    Entity* remoteEntity = assets.CreateRemotePlayer(remotePlayerDto);

    // Guardamos el remoto en el mapa.
    // RemotePlayer no es dueño de la entidad; solo la referencia.
    remotePlayers.emplace(entityId,RemotePlayer(entityId, remoteEntity));
    std::cout << "[CLIENT_WORLD] RemotePlayer spawneado. id="
              << entityId
              << " pos=("
              << remotePlayerDto.xpos
              << ", "
              << remotePlayerDto.ypos
              << ")"
              << std::endl;
}

void ClientGameWorld::removeRemotePlayer(uint32_t entityId) {
    // Buscamos el jugador remoto.
    auto it = remotePlayers.find(entityId);

    // Si no existe, no hay nada que eliminar.
    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD] removeRemotePlayer ignorado. No existe id="
                  << entityId
                  << std::endl;
        return;
    }

    // Sacamos el wrapper RemotePlayer del mapa.
    // Nota: esto NO destruye la Entity del ECS.
    // Si tu ECS necesita destruir/desactivar entidades, luego hay que agregarlo.
    remotePlayers.erase(it);

    std::cout << "[CLIENT_WORLD] RemotePlayer removido. id="
              << entityId
              << std::endl;
}

void ClientGameWorld::updateLocalPlayerPosition(float x, float y) {
    // Si por algún error el local todavía no existe, cortamos.
    if (localPlayer == nullptr) {
        std::cout << "[CLIENT_WORLD][WARN] localPlayer es nullptr."
                  << std::endl;
        return;
    }

    // Obtenemos el TransformComponent del jugador local.
    auto& transform = localPlayer->getComponent<TransformComponent>();

    // Aplicamos la posición enviada por el servidor.
    transform.position.x = x;
    transform.position.y = y;
}

void ClientGameWorld::updateRemotePlayerPosition(uint32_t entityId,float x,float y) {
    // Buscamos el jugador remoto.
    auto it = remotePlayers.find(entityId);

    // Si no existe, NO lo creamos.
    // El movimiento no debe crear entidades.
    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD][WARN] Movimiento recibido para remoto no spawneado. id="
                  << entityId
                  << " pos=(" << x << ", " << y << ")"
                  << std::endl;
        return;
    }

    // Si existe, actualizamos su posición.
    it->second.setPosition(x, y);
}

void ClientGameWorld::updatePlayerPosition(uint32_t entityId,float x,float y) {
    if (isLocalPlayer(entityId)) {
        // Movimiento/corrección del jugador local.
        updateLocalPlayerPosition(x, y);

        std::cout << "[CLIENT_WORLD] sync local id="
                  << entityId
                  << " pos=(" << x << ", " << y << ")"
                  << std::endl;

        return;
    }

    // Movimiento de un jugador remoto existente.
    updateRemotePlayerPosition(entityId, x, y);

    std::cout << "[CLIENT_WORLD] sync remote id="
              << entityId
              << " pos=(" << x << ", " << y << ")"
              << std::endl;
}
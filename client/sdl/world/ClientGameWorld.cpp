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

void ClientGameWorld::updateLocalPlayerPosition(const float x, const float y) {
    // Si por algún error el local todavía no existe, cortamos.
    if (localPlayer == nullptr) {
        std::cout << "[CLIENT_WORLD][WARN] localPlayer es nullptr."
                  << std::endl;
        return;
    }
    std::cout << "[LOCAL SYNC] pos=("
      << x << ", " << y << ")"
      << std::endl;

    // Obtenemos el TransformComponent del jugador local.
    auto& transform = localPlayer->getComponent<TransformComponent>();

    // Aplicamos la posición enviada por el servidor.
    transform.position.x = x;
    transform.position.y = y;
    std::cout << "[LOCAL SYNC] pos=("
          << x << ", " << y << ")"
          << std::endl;
}

void ClientGameWorld::updateRemotePlayerPosition( uint32_t entityId, float x, float y, Direction direction, bool moving) {
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD][WARN] Movimiento recibido para remoto no spawneado. id="
                  << entityId
                  << std::endl;
        return;
    }

    it->second.setPositionAndAnimation(x, y, direction, moving);
}


void ClientGameWorld::updatePlayerPosition( uint32_t entityId,float x,float y,Direction direction,bool moving) {
    if (isLocalPlayer(entityId)) {
        updateLocalPlayerPosition(x, y);
        return;
    }

    updateRemotePlayerPosition(entityId, x, y, direction, moving);
}
void ClientGameWorld::updateRemotePlayerEquipment( uint32_t entityId,const EquipmentDto& equipment,const ItemCatalog& itemCatalog) {
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD][WARN] Equipment update para remoto no spawneado. id="
                  << entityId
                  << std::endl;
        return;
    }

    it->second.setEquipment(equipment,itemCatalog);

    std::cout << "[CLIENT_WORLD] Equipment remoto actualizado. id="
              << entityId
              << std::endl;
}
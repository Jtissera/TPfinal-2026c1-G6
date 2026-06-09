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

    if (remoteEntity == nullptr) {
        std::cout << "[CLIENT_WORLD][ERROR] No se pudo crear RemotePlayer. id="
                  << entityId
                  << std::endl;
        return;
    }

    // Guardamos el remoto en el mapa.
    // RemotePlayer no es dueño de la entidad; solo la referencia.
    remotePlayers.emplace(entityId,RemotePlayer(entityId, remoteEntity,remotePlayerDto));

    std::cout << "[CLIENT_WORLD] RemotePlayer spawneado. id="
              << entityId
              << " pos=("
              << remotePlayerDto.xpos
              << ", "
              << remotePlayerDto.ypos
              << ") esFantasma="
              << remotePlayerDto.esFantasma
              << std::endl;

    if (hasRemotePlayer(entityId)) {
        auto it = remotePlayers.find(entityId);

        if (it != remotePlayers.end()) {
            RemotePlayer& remotePlayer = it->second;

            // Actualizamos el DTO guardado.
            remotePlayer.updateDto(remotePlayerDto);

            std::cout << "[CLIENT_WORLD] RemotePlayer ya existe. id="
                      << entityId
                      << " esFantasma="
                      << remotePlayerDto.esFantasma
                      << std::endl;

            // Si el server informa que ahora es fantasma, aplicamos ghost.
            if (remotePlayerDto.esFantasma) {
                applyRemotePlayerGhostState(entityId);
                return;
            }

            // Si antes estaba ghost y ahora el DTO dice vivo,
            // restauramos apariencia normal.
            if (remotePlayer.isGhost()) {
                applyRemotePlayerAliveState(entityId);
                return;
            }
        }

        return;
    }
}

void ClientGameWorld::removeRemotePlayer(uint32_t entityId) {
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD] removeRemotePlayer ignorado. No existe id="
                  << entityId
                  << std::endl;
        return;
    }

    Entity* entity = it->second.getEntity();
    if (entity != nullptr) {
        entity->destroy(); 
    }

    remotePlayers.erase(it);

    std::cout << "[CLIENT_WORLD] RemotePlayer removido de la lista y destruido del ECS. id="
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
void ClientGameWorld::updateRemotePlayerEquipment(uint32_t entityId,const EquipmentDto& equipment,const ItemCatalog& itemCatalog) {
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end()) {
        std::cout << "[CLIENT_WORLD][WARN] Equipment update para remoto no spawneado. id="
                  << entityId
                  << std::endl;
        return;
    }

    RemotePlayer& remotePlayer = it->second;

    // Si el remoto está en ghost, ignoramos updates visuales de equipamiento.
    // Esto evita que EquipmentComponent vuelva a aplicar body_sheet encima del ghost.
    if (remotePlayer.isGhost()) {
        std::cout << "[CLIENT_WORLD] Equipment remoto ignorado porque es ghost. id="
                  << entityId
                  << std::endl;
        return;
    }

    remotePlayer.setEquipment(equipment, itemCatalog);

    std::cout << "[CLIENT_WORLD] Equipment remoto actualizado. id="
              << entityId
              << std::endl;
}

void ClientGameWorld::appendRemoteAttackTargets(
    std::vector<AttackTarget>& targets,
    uint8_t localPlayerLevel
) {
    for (auto& [remotePlayerId, remotePlayer] : remotePlayers) {
        if (remotePlayer.isGhost()) {
            continue;
        }

        const uint8_t remoteLevel = remotePlayer.getLevel();

        if (!canAttackByFairPlay(localPlayerLevel, remoteLevel)) {
            continue;
        }

        Entity* entity = remotePlayer.getEntity();

        if (entity == nullptr) {
            continue;
        }

        targets.push_back(AttackTarget{remotePlayerId, entity});
    }
}

void ClientGameWorld::applyRemotePlayerGhostState(uint32_t playerId) {
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end()) {
        std::cout << "[REMOTE_PLAYER] ghost ignorado, no existe id="
                  << playerId
                  << std::endl;
        return;
    }

    RemotePlayer& remotePlayer = it->second;
    Entity* remote = remotePlayer.getEntity();

    if (remote == nullptr) {
        std::cout << "[REMOTE_PLAYER] ghost ignorado, entity null id="
                  << playerId
                  << std::endl;
        return;
    }

    // Estado lógico local del remoto.
    remotePlayer.setGhost(true);

    // Primero limpiamos equipamiento visual.
    // No usar clear(), porque puede restaurar body_sheet.
    if (remote->hasComponent<EquipmentComponent>()) {
        auto& equipment = remote->getComponent<EquipmentComponent>();

        equipment.setWeapon(std::nullopt);
        equipment.setShield(std::nullopt);
        equipment.setArmor(std::nullopt);
        equipment.setHelmet(std::nullopt);
    }

    // Ghost al final para que nada lo pise.
    assets.applyGhostAppearance(*remote);

    std::cout << "[REMOTE_PLAYER] playerId="
              << playerId
              << " pasó a fantasma"
              << std::endl;
}

bool ClientGameWorld::isRemotePlayerGhost(uint32_t playerId) const {
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end()) {
        return false;
    }

    return it->second.isGhost();
}

void ClientGameWorld::applyRemotePlayerAliveState(uint32_t playerId) {
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end()) {
        std::cout << "[REMOTE_PLAYER] alive ignorado, no existe id="
                  << playerId
                  << std::endl;
        return;
    }

    RemotePlayer& remotePlayer = it->second;
    Entity* remote = remotePlayer.getEntity();

    if (remote == nullptr) {
        std::cout << "[REMOTE_PLAYER] alive ignorado, entity null id="
                  << playerId
                  << std::endl;
        return;
    }

    // Desde ahora vuelve a ser targeteable y acepta equipment updates.
    remotePlayer.setGhost(false);

    // Restauramos apariencia normal usando el DTO guardado.
    assets.applyRemotePlayerAppearance(*remote, remotePlayer.getDto());

    std::cout << "[REMOTE_PLAYER] playerId="
              << playerId
              << " volvió a cuerpo normal"
              << std::endl;
}


bool ClientGameWorld::canAttackByFairPlay(uint32_t myLevel, uint32_t targetLevel) {
    constexpr uint32_t newbieMaxLevel = 12;
    constexpr uint32_t maxLevelDiff = 10;

    // Newbies no atacan ni son atacados.
    if (myLevel <= newbieMaxLevel || targetLevel <= newbieMaxLevel) {
        return false;
    }

    const int diff = std::abs(
        static_cast<int>(myLevel) - static_cast<int>(targetLevel)
    );

    // Diferencia mayor a 10 niveles: no se permite PvP.
    return diff <= static_cast<int>(maxLevelDiff);
}

void ClientGameWorld::updateRemotePlayerLevel(
    uint32_t playerId,
    uint8_t newLevel
) {
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end()) {
        std::cout << "[REMOTE_LEVEL] ignorado, no existe id="
                  << playerId
                  << " level="
                  << newLevel
                  << std::endl;
        return;
    }

    it->second.setLevel(newLevel);

    std::cout << "[REMOTE_LEVEL] playerId="
              << playerId
              << " newLevel="
              << newLevel
              << std::endl;
}
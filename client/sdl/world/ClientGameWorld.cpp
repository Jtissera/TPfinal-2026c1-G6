#include "ClientGameWorld.h"

#include <iostream>

#include "../ECS/Components.h"

ClientGameWorld::ClientGameWorld(uint32_t localPlayerId, Entity *localPlayer, AssetManager &assets)
    : localPlayerId(localPlayerId),
      localPlayer(localPlayer),
      assets(assets)
{
}

bool ClientGameWorld::isLocalPlayer(uint32_t entityId) const
{
    // Compara el id recibido por red contra el id local.
    return entityId == localPlayerId;
}

bool ClientGameWorld::hasRemotePlayer(uint32_t entityId) const
{
    // Busca si el remoto ya existe en el mapa.
    return remotePlayers.find(entityId) != remotePlayers.end();
}

void ClientGameWorld::spawnRemotePlayer(const PlayerDto &remotePlayerDto)
{
    const uint32_t entityId = static_cast<uint32_t>(remotePlayerDto.playerID);

    if (isLocalPlayer(entityId))
    {
        return;
    }

    auto it = remotePlayers.find(entityId);
    if (it != remotePlayers.end())
    {
        RemotePlayer &remotePlayer = it->second;

        remotePlayer.updateDto(remotePlayerDto);

        if (remotePlayerDto.esFantasma)
        {
            applyRemotePlayerGhostState(entityId);
        }
        else if (remotePlayer.isGhost())
        {
            applyRemotePlayerAliveState(entityId);
        }
        return;
    }

    Entity *remoteEntity = assets.CreateRemotePlayer(remotePlayerDto);

    if (remoteEntity == nullptr)
    {
        std::cout << "[CLIENT_WORLD][ERROR] No se pudo crear RemotePlayer. id="
                  << entityId << std::endl;
        return;
    }

    remotePlayers.emplace(entityId, RemotePlayer(entityId, remoteEntity, remotePlayerDto));

    std::cout << "[CLIENT_WORLD] RemotePlayer spawneado de cero. id=" << entityId << std::endl;

    if (remotePlayerDto.esFantasma)
    {
        applyRemotePlayerGhostState(entityId);
    }
}

void ClientGameWorld::removeRemotePlayer(uint32_t entityId)
{
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end())
    {
        std::cout << "[CLIENT_WORLD] removeRemotePlayer ignorado. No existe id="
                  << entityId << std::endl;
        return;
    }

    Entity *entity = it->second.getEntity();
    if (entity != nullptr)
    {
        entity->destroy();
    }

    remotePlayers.erase(it);

    std::cout << "[CLIENT_WORLD] RemotePlayer removido de la lista y destruido del ECS. id="
              << entityId
              << std::endl;
}

void ClientGameWorld::updateLocalPlayerPosition(const float x, const float y)
{
    // Si por algún error el local todavía no existe, cortamos.
    if (localPlayer == nullptr)
    {
        return;
    }

    // Obtenemos el TransformComponent del jugador local.
    auto &transform = localPlayer->getComponent<TransformComponent>();

    // Aplicamos la posición enviada por el servidor.
    transform.position.x = x;
    transform.position.y = y;
}

void ClientGameWorld::updateRemotePlayerPosition(uint32_t entityId, float x, float y, Direction direction, bool moving)
{
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end())
    {
        return;
    }

    it->second.setPositionAndAnimation(x, y, direction, moving);
}

void ClientGameWorld::updatePlayerPosition(uint32_t entityId, float x, float y, Direction direction, bool moving)
{
    if (isLocalPlayer(entityId))
    {
        updateLocalPlayerPosition(x, y);
        return;
    }

    updateRemotePlayerPosition(entityId, x, y, direction, moving);
}
void ClientGameWorld::updateRemotePlayerEquipment(uint32_t entityId, const EquipmentDto &equipment, const ItemCatalog &itemCatalog)
{
    auto it = remotePlayers.find(entityId);

    if (it == remotePlayers.end())
    {
        std::cout << "[CLIENT_WORLD][WARN] Equipment update para remoto no spawneado. id="
                  << entityId
                  << std::endl;
        return;
    }

    RemotePlayer &remotePlayer = it->second;

    // Si el remoto está en ghost, ignoramos updates visuales de equipamiento.
    // Esto evita que EquipmentComponent vuelva a aplicar body_sheet encima del ghost.
    if (remotePlayer.isGhost())
    {
        std::cout << "[CLIENT_WORLD] Equipment remoto ignorado porque es ghost. id="
                  << entityId
                  << std::endl;
        return;
    }

    remotePlayer.setEquipment(equipment, itemCatalog);


}

void ClientGameWorld::appendRemoteAttackTargets(
    std::vector<AttackTarget> &targets)
{
    for (auto &[remotePlayerId, remotePlayer] : remotePlayers)
    {
        if (remotePlayer.isGhost())
            continue;

        Entity *entity = remotePlayer.getEntity();
        if (entity == nullptr)
            continue;

        targets.push_back(AttackTarget{remotePlayerId, entity});
    }
}

void ClientGameWorld::applyRemotePlayerGhostState(uint32_t playerId)
{
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end())
    {
        std::cout << "[REMOTE_PLAYER] ghost ignorado, no existe id="
                  << playerId
                  << std::endl;
        return;
    }

    RemotePlayer &remotePlayer = it->second;
    Entity *remote = remotePlayer.getEntity();

    if (remote == nullptr)
    {
        std::cout << "[REMOTE_PLAYER] ghost ignorado, entity null id="
                  << playerId
                  << std::endl;
        return;
    }

    // Estado lógico local del remoto.
    remotePlayer.setGhost(true);

    // Primero limpiamos equipamiento visual.
    // No usar clear(), porque puede restaurar body_sheet.
    if (remote->hasComponent<EquipmentComponent>())
    {
        auto &equipment = remote->getComponent<EquipmentComponent>();

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

bool ClientGameWorld::isRemotePlayerGhost(uint32_t playerId) const
{
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end())
    {
        return false;
    }

    return it->second.isGhost();
}

void ClientGameWorld::applyRemotePlayerAliveState(uint32_t playerId)
{
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end())
    {
        std::cout << "[REMOTE_PLAYER] alive ignorado, no existe id="
                  << playerId
                  << std::endl;
        return;
    }

    RemotePlayer &remotePlayer = it->second;
    Entity *remote = remotePlayer.getEntity();

    if (remote == nullptr)
    {
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

bool ClientGameWorld::canAttackByFairPlay(uint32_t myLevel, uint32_t targetLevel)
{
    constexpr uint32_t newbieMaxLevel = 12;
    constexpr uint32_t maxLevelDiff = 10;

    // Newbies no atacan ni son atacados.
    if (myLevel <= newbieMaxLevel || targetLevel <= newbieMaxLevel)
    {
        return false;
    }

    const int diff = std::abs(
        static_cast<int>(myLevel) - static_cast<int>(targetLevel));

    // Diferencia mayor a 10 niveles: no se permite PvP.
    return diff <= static_cast<int>(maxLevelDiff);
}

void ClientGameWorld::updateRemotePlayerLevel(
    uint32_t playerId,
    uint8_t newLevel)
{
    auto it = remotePlayers.find(playerId);

    if (it == remotePlayers.end())
    {
        return;
    }

    it->second.setLevel(newLevel);

    std::cout << "[REMOTE_LEVEL] playerId="
              << playerId
              << " newLevel="
              << newLevel
              << std::endl;
}

std::vector<Entity *> ClientGameWorld::getRemotePlayerEntities() const
{
    std::vector<Entity *> result;
    result.reserve(remotePlayers.size());

    for (const auto &[id, remotePlayer] : remotePlayers)
    {
        Entity *entity = const_cast<RemotePlayer &>(remotePlayer).getEntity();
        if (entity != nullptr)
        {
            result.push_back(entity);
        }
    }

    return result;
}

Entity *ClientGameWorld::getRemotePlayerEntity(uint32_t entityId) const
{
    auto it = remotePlayers.find(entityId);
    if (it == remotePlayers.end())
        return nullptr;
    return const_cast<RemotePlayer &>(it->second).getEntity();
}

void ClientGameWorld::updateRemotePlayerHealth(uint32_t entityId,
                                               int hp,
                                               int hpMax)
{
    // Buscamos al jugador remoto por ID.
    auto it = remotePlayers.find(entityId);

    // Si no existe como remoto, no hacemos nada.
    if (it == remotePlayers.end())
    {
        return;
    }

    // Delegamos la actualización al RemotePlayer.
    it->second.setHealth(hp, hpMax);
}
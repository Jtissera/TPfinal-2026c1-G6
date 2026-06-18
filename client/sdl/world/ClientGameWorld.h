
#ifndef TALLER_TP_CLIENTGAMEWORLD_H
#define TALLER_TP_CLIENTGAMEWORLD_H

#include <cstdint>
#include <map>

#include "RemotePlayer.h"
#include "../AssetManager.h"
#include "../../../common/dtos/gameTypes.h"
#include "common/dtos/equipmentDto.h"
#include "client/sdl/AttackSystem.h"

class ClientGameWorld
{
private:
    uint32_t localPlayerId;
    // Entidad visual del jugador local.
    // No es dueña: la administra Manager.
    Entity *localPlayer;

    // AssetManager usado para crear entidades visuales.
    AssetManager &assets;
    std::map<uint32_t, RemotePlayer> remotePlayers;

    bool canAttackByFairPlay(uint32_t myLevel, uint32_t targetLevel);

public:
    // Constructor del mundo cliente.
    ClientGameWorld(
        uint32_t localPlayerId,
        Entity *localPlayer,
        AssetManager &assets);

    // Devuelve true si el id pertenece al jugador local.
    bool isLocalPlayer(uint32_t entityId) const;

    // Devuelve true si ya existe ese jugador remoto.
    bool hasRemotePlayer(uint32_t entityId) const;

    std::vector<Entity *> getRemotePlayerEntities() const;

    // Crea un jugador remoto a partir de datos enviados por el server.
    // Esto debe llamarse cuando llegue un PlayerSpawnMessage / PlayerJoinedMessage.
    void spawnRemotePlayer(const PlayerDto &remotePlayerDto);

    // Elimina un jugador remoto si el server avisa que se desconectó.
    void removeRemotePlayer(uint32_t entityId);

    // Actualiza la posición del jugador local.
    void updateLocalPlayerPosition(float x, float y);

    // Actualiza la posición de un remoto EXISTENTE.
    // No crea jugadores. Si no existe, loguea warning.
    void updateRemotePlayerPosition(uint32_t entityId, float x, float y, Direction direction, bool moving);

    // Entrada general para movimientos de jugadores.
    void updatePlayerPosition(uint32_t entityId, float x, float y, Direction direction, bool moving);

    void updateRemotePlayerEquipment(uint32_t entityId, const EquipmentDto &equipment, const ItemCatalog &itemCatalog);

    void appendRemoteAttackTargets(std::vector<AttackTarget> &targets);

    // Aplica visualmente el estado fantasma a un jugador remoto.
    // No toca inventario real; solo cambia sprite y oculta equipamiento visual.
    void applyRemotePlayerGhostState(uint32_t playerId);

    bool isRemotePlayerGhost(uint32_t playerId) const;

    void applyRemotePlayerAliveState(uint32_t playerId);

    void updateRemotePlayerLevel(uint32_t playerId, uint8_t newLevel);

    Entity *getRemotePlayerEntity(uint32_t entityId) const;
};

#endif // TALLER_TP_CLIENTGAMEWORLD_H

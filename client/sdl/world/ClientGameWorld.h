
#ifndef TALLER_TP_CLIENTGAMEWORLD_H
#define TALLER_TP_CLIENTGAMEWORLD_H

#include <cstdint>
#include <map>

#include "RemotePlayer.h"
#include "../AssetManager.h"
#include "../../../common/dtos/gameTypes.h"


class ClientGameWorld {
private:

    uint32_t localPlayerId;
    // Entidad visual del jugador local.
    // No es dueña: la administra Manager.
    Entity* localPlayer;

    // AssetManager usado para crear entidades visuales.
    AssetManager& assets;
    std::map<uint32_t, RemotePlayer> remotePlayers;

public:
    // Constructor del mundo cliente.
    ClientGameWorld(
        uint32_t localPlayerId,
        Entity* localPlayer,
        AssetManager& assets
    );

    // Devuelve true si el id pertenece al jugador local.
    bool isLocalPlayer(uint32_t entityId) const;

    // Devuelve true si ya existe ese jugador remoto.
    bool hasRemotePlayer(uint32_t entityId) const;

    // Crea un jugador remoto a partir de datos enviados por el server.
    // Esto debe llamarse cuando llegue un PlayerSpawnMessage / PlayerJoinedMessage.
    void spawnRemotePlayer(const PlayerDto& remotePlayerDto);

    // Elimina un jugador remoto si el server avisa que se desconectó.
    void removeRemotePlayer(uint32_t entityId);

    // Actualiza la posición del jugador local.
    void updateLocalPlayerPosition(float x, float y);

    // Actualiza la posición de un remoto EXISTENTE.
    // No crea jugadores. Si no existe, loguea warning.
    void updateRemotePlayerPosition(uint32_t entityId, float x, float y,Direction direction,bool moving);

    // Entrada general para movimientos de jugadores.
    void updatePlayerPosition(uint32_t entityId, float x, float y,Direction direction,bool moving);
};




#endif //TALLER_TP_CLIENTGAMEWORLD_H

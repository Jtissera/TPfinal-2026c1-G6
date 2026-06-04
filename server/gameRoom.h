#pragma once

#include <cstdint>
#include <string>

#include "world/gameWorld.h"
#include "../common/queue.h"
#include "clientMessage.h"
#include "gameLoop.h"
#include "monitorQueues.h"
#include "game/Player.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "server/game/equipmentDtoFactory.h"

class GameRoom {
public:
    GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers, NpcFactory& npcFactory,ItemRepository& itemRepo);

    void addClient(uint32_t clientId,
                   Queue<std::shared_ptr<const Message>>& clientQueue);

    void addPlayer(Player player);
    void syncPlayerJoin(uint32_t newPlayerId);
    void removeClient(uint32_t clientId);

    uint32_t getId() const;
    const std::string& getName() const;
    uint8_t getPlayerCount() const;
    uint8_t getMaxPlayers() const;
    bool isFull() const;

    Queue<ClientMessage>& getGameQueue();


    void start();
    void stop();
    void join();

private:
    uint32_t gameId;
    std::string gameName;
    uint8_t maxPlayers;
    Monitor monitor;
    Queue<ClientMessage> gameQueue;
    GameWorld world;
    GameLoop gameLoop;

    void sendExistingPlayersTo(uint32_t newClientId);
    void broadcastPlayerSpawn(uint32_t playerId);
    PlayerDto buildPlayerDto(const Player& player) const;
    void sendInventoryTo(uint32_t playerId);


};
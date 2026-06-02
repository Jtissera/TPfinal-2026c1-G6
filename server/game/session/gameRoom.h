#pragma once

#include "../../../common/network/messages/message.h"
#include "../../../common/queue.h"
#include "../../monitorQueues.h"

#include "../../npc/npcFactory.h"

#include "../../lobby/instanceTransitionEvent.h"
#include "../../lobby/leaveEvent.h"

#include "../items/itemRepository.h"

#include "../../world/gameWorld.h"

#include "gameLoop.h"

#include <memory>
#include <string>

class GameRoom
{
public:
    GameRoom(
        uint32_t gameId,
        std::string gameName,
        uint8_t maxPlayers,
        NpcFactory &npcFactory,
        ItemRepository &itemRepo,
        Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
        Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
        const toml::table &config,
        std::string mapPath = "assets/sprites/MapAssets/mapa.argmap",
        bool isInstance = false,
        uint32_t originRoomId = 0);

    void addClient(
        uint32_t clientId,
        Queue<std::shared_ptr<const Message>> &clientQueue);

    void addPlayer(uint32_t clientId, Player player);

    void removeClient(uint32_t clientId);

    Queue<ClientMessage> &getGameQueue();

    uint32_t getId() const;

    const std::string &getName() const;

    uint8_t getPlayerCount() const;

    uint8_t getMaxPlayers() const;

    bool isFull() const;

    void start();

    void stop();

    void join();

    bool getIsInstance() const { return isInstance; }

    uint32_t getOriginRoomId() const { return originRoomId; }

private:
    uint32_t gameId;
    std::string gameName;
    uint8_t maxPlayers;

    bool isInstance;
    uint32_t originRoomId;
    std::string mapPath;

    Monitor monitor;

    Queue<ClientMessage> gameQueue;

    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue;

    GameWorld world;

    GameLoop gameLoop;

    int tileSize;
};
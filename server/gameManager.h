#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

#include "../common/queue.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "gameRoom.h"

class GameManager
{
public:
    GameManager();

    uint32_t createGame(const std::string &gameName, uint8_t maxPlayers);

    bool joinGame(uint32_t gameId,
                  uint32_t clientId,
                  Queue<std::shared_ptr<const Message>> &clientQueue,Player player);

    void removeClient(uint32_t clientId);

    std::vector<GameInfo> listGames() const;

    void stopAll();

    Queue<ClientMessage> &getGameQueue(uint32_t gameId);

private:
    mutable std::mutex mutex;
    uint32_t nextGameId = 1;

    std::unordered_map<uint32_t, std::unique_ptr<GameRoom>> rooms;

    // clientId -> gameId para saber en qué sala está cada cliente
    std::unordered_map<uint32_t, uint32_t> clientRoom;
};
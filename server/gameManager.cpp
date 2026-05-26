#include "gameManager.h"

GameManager::GameManager() {}

uint32_t GameManager::createGame(const std::string &gameName, uint8_t maxPlayers)
{
    std::unique_lock<std::mutex> lock(mutex);

    uint32_t id = nextGameId++;
    auto room = std::make_unique<GameRoom>(id, gameName, maxPlayers);
    room->start();
    rooms.emplace(id, std::move(room));

    std::cout << "[GameManager] created game id=" << id
              << " name=" << gameName
              << " maxPlayers=" << static_cast<int>(maxPlayers) << std::endl;

    return id;
}

bool GameManager::joinGame(uint32_t gameId,
                           uint32_t clientId,
                           Queue<std::shared_ptr<const Message>> &clientQueue,
                           Player player)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto it = rooms.find(gameId);
    if (it == rooms.end())
        return false;

    if (it->second->isFull())
        return false;

    it->second->addClient(clientId, clientQueue, std::move(player));
    clientRoom[clientId] = gameId;

    std::cout << "[GameManager] client=" << clientId
              << " joined game=" << gameId << std::endl;

    return true;
}
void GameManager::removeClient(uint32_t clientId)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto it = clientRoom.find(clientId);
    if (it == clientRoom.end())
        return;

    uint32_t gameId = it->second;
    clientRoom.erase(it);

    auto roomIt = rooms.find(gameId);
    if (roomIt != rooms.end())
        roomIt->second->removeClient(clientId);
}

std::vector<GameInfo> GameManager::listGames() const
{
    std::unique_lock<std::mutex> lock(mutex);

    std::vector<GameInfo> result;
    result.reserve(rooms.size());

    for (const auto &pair : rooms)
    {
        GameInfo info;
        info.gameId = pair.second->getId();
        info.gameName = pair.second->getName();
        info.playerCount = pair.second->getPlayerCount();
        info.maxPlayers = pair.second->getMaxPlayers();
        result.push_back(std::move(info));
    }

    return result;
}

void GameManager::stopAll()
{
    std::unique_lock<std::mutex> lock(mutex);

    for (auto &pair : rooms)
    {
        pair.second->stop();
        pair.second->join();
    }
    rooms.clear();
    clientRoom.clear();
}

Queue<ClientMessage> &GameManager::getGameQueue(uint32_t gameId)
{
    std::unique_lock<std::mutex> lock(mutex);
    return rooms.at(gameId)->getGameQueue();
}
#include "gameManager.h"

GameManager::GameManager(
    NpcFactory &npcFactory,
    ItemRepository &itemRepo,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    const toml::table &config)
    : npcFactory(npcFactory),
      itemRepo(itemRepo),
      leaveQueue(leaveQueue),
      transitionQueue(transitionQueue),
      config(config)
{
}

uint32_t GameManager::createGame(
    const std::string &gameName,
    uint8_t maxPlayers)
{
    std::unique_lock<std::mutex> lock(mutex);

    uint32_t id = nextGameId++;

    auto room = std::make_unique<GameRoom>(
        id,
        gameName,
        maxPlayers,
        npcFactory,
        itemRepo,
        leaveQueue,
        transitionQueue,
        config);

    room->start();

    rooms.emplace(id, std::move(room));

    return id;
}

uint32_t GameManager::getOrCreateInstance(
    const std::string &mapPath,
    uint32_t originRoomId)
{
    std::unique_lock<std::mutex> lock(mutex);

    // Buscar instancia existente
    for (const auto &[id, room] : rooms)
    {
        if (room->getIsInstance() &&
            room->getName() == mapPath)
        {
            return id;
        }
    }

    // Crear nueva instancia
    uint32_t id = nextGameId++;

    auto room = std::make_unique<GameRoom>(
        id,
        mapPath,
        255, // maxPlayers por defecto para instancias
        npcFactory,
        itemRepo,
        leaveQueue,
        transitionQueue,
        config,
        mapPath,
        true,
        originRoomId);

    room->start();

    rooms.emplace(id, std::move(room));

    return id;
}

void GameManager::cleanEmptyInstances()
{
    std::vector<uint32_t> toRemove;

    for (const auto &[id, room] : rooms)
    {
        if (room->getIsInstance() &&
            room->getPlayerCount() == 0)
        {
            toRemove.push_back(id);
        }
    }

    for (uint32_t id : toRemove)
    {
        rooms[id]->stop();
        rooms[id]->join();
        rooms.erase(id);
    }
}

bool GameManager::joinGame(
    uint32_t gameId,
    uint32_t clientId,
    Queue<std::shared_ptr<const Message>> &clientQueue)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto it = rooms.find(gameId);

    if (it == rooms.end())
        return false;

    if (it->second->isFull())
        return false;

    it->second->addClient(clientId, clientQueue);

    clientRoom[clientId] = gameId;

    std::cout
        << "[GameManager] client="
        << clientId
        << " joined game="
        << gameId
        << std::endl;

    return true;
}

void GameManager::addPlayerToGame(
    uint32_t gameId,
    uint32_t clientId,
    Player player)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto it = rooms.find(gameId);

    if (it != rooms.end())
    {
        it->second->addPlayer(
            clientId,
            std::move(player));
    }
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
    {
        roomIt->second->removeClient(clientId);
    }

    cleanEmptyInstances();
}

std::vector<GameInfo> GameManager::listGames() const
{
    std::unique_lock<std::mutex> lock(mutex);

    std::vector<GameInfo> result;

    for (const auto &[id, room] : rooms)
    {
        // ocultar instancias
        if (room->getIsInstance())
            continue;

        GameInfo info;

        info.gameId = room->getId();
        info.gameName = room->getName();
        info.playerCount = room->getPlayerCount();
        info.maxPlayers = room->getMaxPlayers();

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

uint32_t GameManager::getOriginRoomId(
    uint32_t instanceRoomId) const
{
    std::unique_lock<std::mutex> lock(mutex);

    auto it = rooms.find(instanceRoomId);

    if (it == rooms.end())
        return 0;

    return it->second->getOriginRoomId();
}
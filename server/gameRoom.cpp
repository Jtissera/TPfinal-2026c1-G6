#include "gameRoom.h"

GameRoom::GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers)
    : gameId(gameId),
      gameName(std::move(gameName)),
      maxPlayers(maxPlayers),
      monitor(),
      gameQueue(),
      world("assets/sprites/MapAssets/mapa.argmap"),
      gameLoop(gameQueue, monitor, world)
       {}

void GameRoom::addClient(uint32_t clientId, Queue<std::shared_ptr<const Message>> &clientQueue)
{
    monitor.addQueue(clientId, clientQueue);
    world.addPlayer(clientId, 6 * 96, 7 * 96); 

}

void GameRoom::removeClient(uint32_t clientId)
{
    monitor.removeQueue(clientId);
    world.removePlayer(clientId);
}

Queue<ClientMessage> &GameRoom::getGameQueue() { return gameQueue; }

uint32_t GameRoom::getId() const { return gameId; }

const std::string &GameRoom::getName() const { return gameName; }

uint8_t GameRoom::getPlayerCount() const { return monitor.size(); }

uint8_t GameRoom::getMaxPlayers() const { return maxPlayers; }

bool GameRoom::isFull() const { return monitor.size() >= maxPlayers; }

void GameRoom::start() { gameLoop.start(); }

void GameRoom::stop() { gameLoop.stop(); }

void GameRoom::join() { gameLoop.join(); }

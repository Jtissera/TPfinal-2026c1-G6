#include "gameRoom.h"

GameRoom::GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers,
                   NpcFactory &npcFactory, ItemRepository &itemRepo,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
                   Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
                   const toml::table &config)
    : gameId(gameId), gameName(std::move(gameName)), maxPlayers(maxPlayers),
      monitor(), gameQueue(), leaveQueue(leaveQueue),
      world("assets/sprites/MapAssets/mapa.argmap", npcFactory, itemRepo,
            config),
            
      gameLoop(gameQueue, monitor, world, leaveQueue, transitionQueue, gameId, config) {}

GameRoom::GameRoom(uint32_t gameId, std::string gameName,
                   const std::string &mapPath, bool isInstance,
                   uint32_t originRoomId,
                   NpcFactory &npcFactory, ItemRepository &itemRepo,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
                   Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
                   const toml::table &config)
    : gameId(gameId),
      gameName(std::move(gameName)),
      maxPlayers(255),
      monitor(), gameQueue()
      isInstance(isInstance),
      originRoomId(originRoomId),
      mapPath(mapPath),
      leaveQueue(leaveQueue),
      world(mapPath, npcFactory, itemRepo, config),
      gameLoop(gameQueue, monitor, world, leaveQueue, transitionQueue, gameId, config),
      tileSize(config["world"]["tile_size"].value_or(96)) 
      {
}
>>>>>>> dev

void GameRoom::addClient(uint32_t clientId,
                         Queue<std::shared_ptr<const Message>> &clientQueue)
{
  monitor.addQueue(clientId, clientQueue);
}

//hay que sacar el id de aca, no va mas, firma desactualizada
//tuve temitas de que stats se enviaban antes de que se hiciera
//cambio de cola, las recibi lobby y desatre

void GameRoom::addPlayer(uint32_t clientId, Player player) {
    world.addPlayer(std::move(player));

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

void GameRoom::start() {
    gameLoop.start();
}

void GameRoom::stop() { gameLoop.stop(); }

void GameRoom::join() { gameLoop.join(); }
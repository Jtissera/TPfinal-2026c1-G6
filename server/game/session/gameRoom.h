#pragma once

#include <cstdint>
#include <string>
#include <toml++/toml.hpp>

#include "../../clientMessage.h"
#include "../../lobby/leaveEvent.h"
#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../common/queue.h"
#include "../player/Player.h"
#include "gameLoop.h"

class GameRoom {
public:
  GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers,
           NpcFactory &npcFactory, ItemRepository &itemRepo,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           const toml::table &config);

  void addClient(uint32_t clientId,
                 Queue<std::shared_ptr<const Message>> &clientQueue);

  void addPlayer(Player player);

  void removeClient(uint32_t clientId);

  uint32_t getId() const;
  const std::string &getName() const;
  uint8_t getPlayerCount() const;
  uint8_t getMaxPlayers() const;
  bool isFull() const;

  Queue<ClientMessage> &getGameQueue();

  void start();
  void stop();
  void join();

private:
  uint32_t gameId;
  std::string gameName;
  uint8_t maxPlayers;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;

  Monitor monitor;
  Queue<ClientMessage> gameQueue;
  GameWorld world;
  GameLoop gameLoop;
};
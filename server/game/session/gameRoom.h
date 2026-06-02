#pragma once

#include <cstdint>
#include <string>
#include <toml++/toml.hpp>
#include <iostream>

#include "../common/network/messages/server/npc/npcListMessage.h"
#include "../../clientMessage.h"
#include "../../lobby/leaveEvent.h"
#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../common/queue.h"
#include "../player/Player.h"
#include "gameLoop.h"

class GameRoom
{
public:
  GameRoom(uint32_t gameId, std::string gameName, uint8_t maxPlayers,
           NpcFactory &npcFactory, ItemRepository &itemRepo,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
           const toml::table &config);

  GameRoom(uint32_t gameId, std::string gameName, const std::string &mapPath,
           bool isInstance, uint32_t originRoomId,
           NpcFactory &npcFactory, ItemRepository &itemRepo,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
           const toml::table &config);

  void addClient(uint32_t clientId,
                 Queue<std::shared_ptr<const Message>> &clientQueue);

void addPlayer(uint32_t clientId, Player player);

  void removeClient(uint32_t clientId);

  uint32_t getId() const;
  const std::string &getName() const;
  uint8_t getPlayerCount() const;
  uint8_t getMaxPlayers() const;
  bool isFull() const;

  Queue<ClientMessage> &getGameQueue();
  bool getIsInstance() const { return isInstance; }
  uint32_t getOriginRoomId() const { return originRoomId; }

  void start();
  void stop();
  void join();

private:

bool initialSnapshotSent = false;
  uint32_t gameId;
  std::string gameName;
  uint8_t maxPlayers;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
int tileSize;
  Monitor monitor;
  Queue<ClientMessage> gameQueue;
  GameWorld world;
  GameLoop gameLoop;
  bool isInstance = false;
  uint32_t originRoomId = 0;
  std::string mapPath;
};
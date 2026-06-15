#pragma once

#include "../../lobby/instanceTransitionEvent.h"
#include "../../lobby/leaveEvent.h"
#include "../../persistence/playerArchive.h"
#include "../common/network/messages/client/movement/moveMessage.h"
#include "../common/network/messages/server/player/EntityDespawnMessage.h"
#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../session/statManager.h"

#include <iostream>
#include <memory>

#include "../../clientMessage.h"
#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "ActionDispatcher.h"

class GameLoop : public Thread {
public:
  GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor, GameWorld &world,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
           uint32_t gameId, const toml::table &config, PlayerArchive &archive,
           const std::string &mapId);
  void run() override;
  void stop() override;

  GameLoop(const GameLoop &) = delete;
  GameLoop &operator=(const GameLoop &) = delete;

private:
  PlayerArchive &archive;
  std::string mapId;
  uint32_t persistTickCounter = 0;
  uint32_t persistEveryNTicks = 0;
  Queue<ClientMessage> &gameQueue;
  Monitor &monitor;
  ActionDispatcher dispatcher;
  GameWorld &world;
  StatManager statManager;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
  uint32_t gameId;
  float tickRateMs;

  Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue;
  void handleInstanceTransition(const GameWorld::InstanceEntry &entry);

  void processMessage(const ClientMessage &incoming);
  void worldUpdate(float deltaSeconds);
  void handleLeaveGame(uint32_t clientId);
};
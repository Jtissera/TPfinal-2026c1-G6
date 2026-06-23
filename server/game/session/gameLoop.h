#pragma once

#include "../../lobby/instanceTransitionEvent.h"
#include "../../lobby/leaveEvent.h"
#include "../../persistence/playerArchive.h"
#include "../common/network/messages/client/movement/moveMessage.h"
#include "../common/network/messages/server/chat/chatNotificationMessage.h"
#include "../common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "../common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "../common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "../common/network/messages/server/npc/npcAttackMessage.h"
#include "../common/network/messages/server/npc/npcMoveMessage.h"
#include "../common/network/messages/server/npc/npcSpawnMessage.h"
#include "../common/network/messages/server/player/EntityDespawnMessage.h"
#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/messages/server/player/playerDiedMessage.h"
#include "../common/network/messages/server/player/playerHealthMessage.h"
#include "../common/network/messages/server/player/playerResurrectedMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../session/statManager.h"
#include "../../clientMessage.h"
#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "ActionDispatcher.h"
#include "../../world/WorldTickResult.h"


#include <fstream>
#include <iostream>
#include <memory>
class ClanManager;

class GameLoop : public Thread
{
public:
  GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor, GameWorld &world,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
           uint32_t gameId, const toml::table &config, PlayerArchive &archive,
           const std::string &mapId, ClanManager &clanManager,
           uint32_t originRoomId);

  void run() override;
  void stop() override;

  GameLoop(const GameLoop &) = delete;
  GameLoop &operator=(const GameLoop &) = delete;

private:
  PlayerArchive &archive;
  std::string mapId;
  const uint32_t originRoomId;
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
  void handleInstanceTransition(const InstanceEntry &entry);
  std::string resolveMapPath(const std::string &targetMap) const;

  void processMessage(const ClientMessage &incoming);
  void worldUpdate(float deltaSeconds);
  void handleLeaveGame(uint32_t clientId);
};
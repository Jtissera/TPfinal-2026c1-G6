#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <toml++/toml.hpp>
#include <unordered_map>
#include <vector>

#include "../../../common/network/messages/server/lobby/gameListMessage.h"
#include "../../../common/queue.h"
#include "../../clientMessage.h"
#include "../../lobby/leaveEvent.h"
#include "../player/Player.h"
#include "gameRoom.h"
#include "../../lobby/instanceTransitionEvent.h"

class GameManager
{
public:
  GameManager(NpcFactory &, ItemRepository &,
              Queue<std::shared_ptr<LeaveEvent>> &,
              Queue<std::shared_ptr<InstanceTransitionEvent>> &,
              const toml::table &);

  uint32_t createGame(const std::string &gameName, uint8_t maxPlayers);

  bool joinGame(uint32_t gameId, uint32_t clientId,
                Queue<std::shared_ptr<const Message>> &clientQueue);

  void addPlayerToGame(uint32_t gameId, Player player);

  void removeClient(uint32_t clientId);

  std::vector<GameInfo> listGames() const;

  void stopAll();

  uint32_t getOriginRoomId(uint32_t instanceRoomId) const;
  uint32_t getOrCreateInstance(const std::string &mapPath, uint32_t originRoomId);
  Queue<ClientMessage> &getGameQueue(uint32_t gameId);
  void broadcastExceptInGame(uint32_t gameId, uint32_t excludeId, const std::shared_ptr<const Message> &msg);
  const GameWorld* getGameWorld(uint32_t gameId) const;
    void syncPlayerJoin(uint32_t gameId, uint32_t playerId);

private:
  const toml::table &config;
  mutable std::mutex mutex;
  uint32_t nextGameId = 1;

  void cleanEmptyInstances();

  std::unordered_map<uint32_t, std::unique_ptr<GameRoom>> rooms;
  std::unordered_map<uint32_t, uint32_t> clientRoom;

  NpcFactory &npcFactory;
  ItemRepository &itemRepo;

  Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
};
#pragma once

#include <cstdint>
#include <string>
#include <toml++/toml.hpp>

#include "../../clientMessage.h"
#include "../../lobby/leaveEvent.h"
#include "../../monitorQueues.h"
#include "../../persistence/playerArchive.h"
#include "../../world/gameWorld.h"
#include "../common/queue.h"
#include "../player/Player.h"
#include "common/dtos/equipmentDto.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/world/EntitySpawnMessage.h"
#include "gameLoop.h"
#include "server/game/equipmentDtoFactory.h"

class GameRoom {
public:
  GameRoom(uint32_t gameId, std::string gameName, const std::string &mapPath,
           bool isInstance, uint32_t originRoomId, NpcFactory &npcFactory,
           ItemRepository &itemRepo,
           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
           const toml::table &config, PlayerArchive &archive);

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
  void broadcastExcept(uint32_t excludeId,
                       const std::shared_ptr<const Message> &msg);
  const GameWorld &getWorld() const;
  bool getIsInstance() const { return isInstance; }
  uint32_t getOriginRoomId() const { return originRoomId; }
  const std::string &getMapPath() const { return mapPath; }
  void syncPlayerJoin(uint32_t newPlayerId);

  void start();
  void stop();
  void join();

private:
  uint32_t gameId;
  std::string gameName;
  uint8_t maxPlayers;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;

  PlayerArchive &archive;

  Monitor monitor;
  Queue<ClientMessage> gameQueue;
  GameWorld world;
  GameLoop gameLoop;
  bool isInstance = false;
  uint32_t originRoomId = 0;
  std::string mapPath;

  void sendExistingPlayersTo(uint32_t newClientId);
  void broadcastPlayerSpawn(uint32_t playerId);
  void sendInventoryTo(uint32_t playerId);
  PlayerDto buildPlayerDto(const Player &player) const;
  void sendExistingNpcsTo(uint32_t clientId);
};
#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/client/auth/createCharMessage.h"
#include "../common/network/messages/client/auth/loginMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/server/auth/connectOKMessage.h"
#include "../common/network/messages/server/auth/createOkMessage.h"
#include "../common/network/messages/server/auth/loginOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/messages/server/lobby/leaveOkMessage.h"
#include "../common/network/messages/server/system/mapChangedMessage.h"
#include "../network/receiver.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "../game/player/playerFactory.h"
#include "../game/session/gameManager.h"
#include "../monitorQueues.h"
#include "../network/receiverRegistry.h"
#include "../persistence/characterArchive.h"
#include "../persistence/playerArchive.h"
#include "instanceTransitionEvent.h"
#include "leaveEvent.h"
#include "playerRepository.h"
#include <toml++/toml.hpp>

class LobbyHandler : public Thread
{
public:
  LobbyHandler(Queue<ClientMessage> &lobbyQueue,
               Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
               Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
               Monitor &lobbyMonitor,
               GameManager &gameManager,
               ReceiverRegistry &receiverRegistry,
               PlayerRepository &playerRepo,
               PlayerFactory &playerFactory,
               PlayerArchive &archive,
               CharacterArchive &characterArchive,
               const toml::table &config);

  void run() override;
  void stop() override;

private:
  Queue<ClientMessage> &lobbyQueue;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
  Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue;
  Monitor &lobbyMonitor;
  GameManager &gameManager;
  ReceiverRegistry &receiverRegistry;
  PlayerRepository &playerRepo;
  PlayerFactory &playerFactory;
  PlayerArchive &archive;
  CharacterArchive &characterArchive;
  const toml::table &config;

  std::unordered_map<uint32_t, std::string> pendingCharacterNames;

  using MemberHandler = void (LobbyHandler::*)(uint32_t, const Message &);
  std::unordered_map<uint8_t, MemberHandler> handlers;

  void initHandlers();

  void handleConnect(uint32_t clientId, const Message &message);
  void handleLogin(uint32_t clientId, const Message &message);
  void handleCreateChar(uint32_t clientId, const Message &message);
  void handleListGames(uint32_t clientId, const Message &message);
  void handleCreateGame(uint32_t clientId, const Message &message);
  void handleJoinGame(uint32_t clientId, const Message &message);
  void handleLeaveGame(LeaveEvent &leaveEvent);
  void handleInstanceTransition(InstanceTransitionEvent &event);

  Player *resolveNewCharPlayer(uint32_t clientId,
                               const std::string &characterName,
                               uint32_t requestedGameId,
                               uint32_t &targetGameId);

  Player *resolveReturningPlayer(uint32_t clientId,
                                 const std::string &characterName,
                                 uint32_t requestedGameId,
                                 uint32_t &targetGameId);

  uint32_t resolveTargetInstance(const std::string &characterName,
                                 uint32_t requestedGameId,
                                 const PlayerSnapshot &snap);

  void finalizeJoin(uint32_t clientId,
                    uint32_t targetGameId,
                    uint32_t requestedGameId,
                    Player &player,
                    Queue<std::shared_ptr<const Message>> &clientQueue);

  void transitionToOrigin(InstanceTransitionEvent &event);
  void transitionToInstance(InstanceTransitionEvent &event);
  void completeTransition(uint32_t targetRoomId,
                          InstanceTransitionEvent &event,
                          const std::string &mapPath,
                          uint32_t originId);

  PlayerDto buildPlayerDto(const Player &player) const;
  std::string findGameName(uint32_t gameId) const;
  std::string buildFullMapPath(const std::string &targetMap) const;
};
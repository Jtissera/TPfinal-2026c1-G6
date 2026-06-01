#pragma once

#include <functional>
#include <iostream>
#include <unordered_map>

#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/client/auth/createCharMessage.h"
#include "../common/network/messages/server/auth/connectOKMessage.h"
#include "../common/network/messages/server/auth/createOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/messages/server/lobby/leaveOkMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "../game/player/playerFactory.h"
#include "../game/session/gameManager.h"
#include "../monitorQueues.h"
#include "../network/receiver.h"
#include "../network/receiverRegistry.h"
#include "leaveEvent.h"
#include "playerRepository.h"

class LobbyHandler : public Thread {
public:
  LobbyHandler(Queue<ClientMessage> &lobbyQueue,
               Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
               Monitor &lobbyMonitor, GameManager &gameManager,
               ReceiverRegistry &receiverRegistry, PlayerRepository &playerRepo,
               PlayerFactory &playerFactory);

  void run() override;
  void stop() override;

private:
  Queue<ClientMessage> &lobbyQueue;
  Queue<std::shared_ptr<LeaveEvent>> &leaveQueue;
  Monitor &lobbyMonitor;
  GameManager &gameManager;
  ReceiverRegistry &receiverRegistry;
  PlayerRepository &playerRepo;
  PlayerFactory &playerFactory;

  using Handler = std::function<void(uint32_t, const Message &)>;
  std::unordered_map<uint8_t, Handler> handlers;

  void initHandlers();

  void handleConnect(uint32_t clientId, const Message &message);
  void handleCreateChar(uint32_t clientId, const Message &message);
  void handleListGames(uint32_t clientId, const Message &message);
  void handleCreateGame(uint32_t clientId, const Message &message);
  void handleJoinGame(uint32_t clientId, const Message &message);
  void handleLeaveGame(LeaveEvent &leaveEvent);
};
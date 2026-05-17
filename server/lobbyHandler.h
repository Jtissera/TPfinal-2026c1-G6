#pragma once

#include <iostream>

#include "../common/thread.h"
#include "../common/queue.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/server/auth/connectOKMessage.h"
#include "clientRegistry.h"
#include "receiverRegistry.h"
#include "gameManager.h"
#include "monitorQueues.h"
#include "receiver.h"

class LobbyHandler : public Thread
{
public:
    LobbyHandler(Queue<ClientMessage> &lobbyQueue,
                 Monitor &lobbyMonitor,
                 GameManager &gameManager,
                 ClientRegistry &clientRegistry,
                 ReceiverRegistry &receiverRegistry);

    void run() override;
    void stop() override;

private:
    Queue<ClientMessage> &lobbyQueue;
    Monitor &lobbyMonitor;
    GameManager &gameManager;
    ClientRegistry &clientRegistry;
    ReceiverRegistry &receiverRegistry;

    void handleConnect(uint32_t clientId, const Message &message);
    void handleListGames(uint32_t clientId);
    void handleCreateGame(uint32_t clientId, const Message &message);
    void handleJoinGame(uint32_t clientId, const Message &message);
};
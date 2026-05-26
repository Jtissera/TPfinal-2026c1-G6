#pragma once
#include "../common/thread.h"
#include "../common/queue.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../common/network/messages/client/auth/createCharMessage.h"
#include "../common/network/messages/server/auth/createOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "playerRepository.h"
#include "game/playerFactory.h"
#include "receiverRegistry.h"
#include "clientRegistry.h"
#include "receiver.h"

class CharHandler : public Thread {
public:
    CharHandler(Queue<ClientMessage>& charQueue,
                Monitor& charMonitor,
                PlayerRepository& playerRepo,
                PlayerFactory& playerFactory,
                ReceiverRegistry& receiverRegistry,
                Queue<ClientMessage>& lobbyQueue,
                ClientRegistry& clientRegistry);

    void run() override;
    void stop() override;

private:
    Queue<ClientMessage>& charQueue;
    Monitor& charMonitor;
    PlayerRepository& playerRepo;
    PlayerFactory& playerFactory;
    ReceiverRegistry& receiverRegistry;
    Queue<ClientMessage>& lobbyQueue;
    ClientRegistry& clientRegistry;
    

    void handleCreateChar(uint32_t clientId, const Message& message);
};
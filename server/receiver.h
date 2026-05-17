#pragma once

#include <cstdint>
#include <atomic>
#include <iostream>

#include "../common/network/protocol/protocol.h"
#include "../common/liberror.h"
#include "../common/network/sockets.h"

#include "clientMessage.h"
#include "../common/queue.h"
#include "../common/thread.h"

class Receiver : public Thread
{
public:
    Receiver(Protocol protocol,
             uint32_t clientId,
             Queue<ClientMessage> &lobbyQueue);

    void setQueue(Queue<ClientMessage> &newQueue);

    void run() override;

private:
    Protocol protocol;
    uint32_t clientId;
    std::atomic<Queue<ClientMessage> *> currentQueue;
};
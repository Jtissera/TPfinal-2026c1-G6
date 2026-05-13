#pragma once

#include <cstdint>
#include <iostream>

#include "../common/network/protocol/protocol.h"
#include "../common/liberror.h"
#include "../common/network/sockets.h"

#include "clientMessage.h"
#include "queue.h"
#include "thread.h"

class Receiver : public Thread
{
public:
    Receiver(Protocol protocol, uint32_t clientId, Queue<ClientMessage> &gameQueue);

    void run() override;

private:
    Protocol protocol;
    uint32_t clientId;
    Queue<ClientMessage> &gameQueue;
};

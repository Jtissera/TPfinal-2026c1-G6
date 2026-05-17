#pragma once

#include <iostream>
#include <memory>

#include "../common/network/messages/message.h"

#include "clientMessage.h"
#include "monitorQueues.h"
#include "../common/queue.h"
#include "../common/thread.h"

class GameLoop : public Thread
{
public:
    GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor);

    void run() override;
    void stop() override;

    GameLoop(const GameLoop &) = delete;
    GameLoop &operator=(const GameLoop &) = delete;

private:
    Queue<ClientMessage> &gameQueue;
    Monitor &monitor;
};

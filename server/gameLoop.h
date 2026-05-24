#pragma once

#include <iostream>
#include <memory>

#include "world/gameWorld.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "../common/queue.h"
#include "../common/thread.h"

class GameLoop : public Thread {
public:
    GameLoop(Queue<ClientMessage>& gameQueue, Monitor& monitor, GameWorld& world);
    void run() override;
    void stop() override;

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;

private:
    Queue<ClientMessage>& gameQueue;
    Monitor& monitor;
    GameWorld& world;
};
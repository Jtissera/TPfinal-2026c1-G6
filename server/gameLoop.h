#pragma once

#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/messages/client/movement/moveMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "statManager.h"

#include <iostream>
#include <memory>

#include "world/gameWorld.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "../common/queue.h"
#include "../common/thread.h"
#include "ActionDispatcher.h"


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
    ActionDispatcher dispatcher;
    GameWorld& world;
    StatManager statManager; //dsp veo esto
    void processMessage(const ClientMessage& incoming);
    void worldUpdate();
};
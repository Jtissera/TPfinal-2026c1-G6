#pragma once

#include <iostream>
#include <memory>

#include "../common/network/messages/message.h"
#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/messages/message.h"
#include "../common/network/messages/client/movement/moveMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../common/dtos/gameTypes.h"
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
    // Estado mockeado del jugador
    uint16_t playerX = 1500;
    uint16_t playerY = 1200;
    static constexpr uint16_t SPEED = 10;
};

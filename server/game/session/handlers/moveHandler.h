#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"

class MoveHandler
{
public:
    void handle(uint32_t clientId,
                const Message &msg,
                GameWorld &world,
                Monitor &monitor);
};
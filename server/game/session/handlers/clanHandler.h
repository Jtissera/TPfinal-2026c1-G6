#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "common/network/messages/internal/clanSyncMessage.h"
#include "common/network/messages/server/clan/clanUpdateMessage.h"

class ClanHandler
{
public:
    void handle(uint32_t clientId,
                const Message &msg,
                GameWorld &world,
                Monitor &monitor);
};
#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/city/cityCommandParser.h"
#include "server/game/stats/gameFormulas.h"
#include "common/network/messages/server/city/npcResponseMessage.h"
#include "common/network/messages/server/error/errorMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "common/network/messages/client/city/interactNpcMessage.h"

class NpcInteractionHandler
{
public:
    explicit NpcInteractionHandler(const toml::table &config);

    void handle(uint32_t clientId,
                const Message &msg,
                GameWorld &world,
                Monitor &monitor);

private:
    GameFormulas formulas;
    CityCommandParser cityCommandParser;

    void sendStats(uint32_t clientId, Player &player, Monitor &monitor);
    void sendInventory(uint32_t clientId, Player &player, Monitor &monitor);
};
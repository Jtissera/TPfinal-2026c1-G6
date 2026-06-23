#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/game/stats/gameFormulas.h"
#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/player/playerHealthMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "../../../world/DeathResult.h"

class CheatHandler
{
public:
    explicit CheatHandler(const toml::table &config);

    void handle(uint32_t clientId,
                const Message &msg,
                GameWorld &world,
                Monitor &monitor);

private:
    GameFormulas formulas;

    void sendStats(uint32_t clientId, Player &player, Monitor &monitor);
    void sendInventory(uint32_t clientId, Player &player, Monitor &monitor);
    void sendDeath(uint32_t clientId, Player &player, Monitor &monitor);
    void sendLevelUpIfNeeded(uint32_t clientId,
                             Player &player,
                             Monitor &monitor);
};
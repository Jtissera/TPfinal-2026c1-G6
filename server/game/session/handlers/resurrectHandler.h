#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/game/stats/gameFormulas.h"
#include "common/network/messages/server/player/entityMoveMessage.h"
#include "common/network/messages/server/player/playerResurrectedMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"

class ResurrectHandler
{
public:
    explicit ResurrectHandler(const toml::table &config);

    void handleResurrect(uint32_t clientId,
                         const Message &msg,
                         GameWorld &world,
                         Monitor &monitor);

    void handleMeditate(uint32_t clientId,
                        const Message &msg,
                        GameWorld &world,
                        Monitor &monitor);

private:
    GameFormulas formulas;

    void sendStats(uint32_t clientId, Player &player, Monitor &monitor);
};
#pragma once
 
#include "../common/network/messages/server/player/playerStatsMessage.h"

#include <memory>

#include "world/gameWorld.h"
#include "monitorQueues.h"
#include "game/gameFormulas.h"

class StatManager {
private:
    GameFormulas formulas;

public:
    void sendPlayerStats(uint32_t clientId,
                         GameWorld& world,
                         Monitor& monitor);
};
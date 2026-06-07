#pragma once

#include "../common/network/messages/server/player/playerStatsMessage.h"

#include <memory>

#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../stats/gameFormulas.h"

class StatManager {
private:
  GameFormulas formulas;

public:
  explicit StatManager(const toml::table& config);
  void sendPlayerStats(uint32_t clientId, GameWorld &world, Monitor &monitor);
};
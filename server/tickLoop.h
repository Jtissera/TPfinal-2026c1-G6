#pragma once
#include <chrono>
#include "../common/thread.h"
#include "world/gameWorld.h"
#include "monitorQueues.h"
#include "game/gameFormulas.h"

class TickLoop : public Thread {
public:
    TickLoop(GameWorld& world, Monitor& monitor);
    void run() override;
    void stop() override;

private:
    static constexpr int   TICK_RATE_MS = 50;
    static constexpr float TICK_SECONDS = TICK_RATE_MS / 1000.0f;

    GameWorld& world;
    Monitor&   monitor;
    GameFormulas formulas;

    void sendStats(uint32_t clientId);
};
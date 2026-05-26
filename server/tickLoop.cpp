#include "tickLoop.h"
#include "../common/network/messages/server/player/playerStatsMessage.h"
#include <thread>

TickLoop::TickLoop(GameWorld& world, Monitor& monitor)
    : world(world), monitor(monitor) {}

void TickLoop::run() {
    using clock = std::chrono::steady_clock;
    auto nextTick = clock::now();

    try {
        while (true) {
            auto changed = world.tick(TICK_SECONDS);
            for (uint32_t id : changed)
                sendStats(id);

            nextTick += std::chrono::milliseconds(TICK_RATE_MS);
            std::this_thread::sleep_until(nextTick);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[TickLoop] error: " << e.what() << std::endl;
    }
}

void TickLoop::stop() {
    Thread::stop();
}

void TickLoop::sendStats(uint32_t clientId) {
    const Player& p = world.getPlayer(clientId);
    auto msg = std::make_shared<const PlayerStatsMessage>(
        p.getLevel(),
        p.getHp(),   p.getMaxHp(),
        p.getMana(), p.getMaxMana(),
        p.getExp(),  formulas.calcExpLimit(p.getLevel()),
        p.getGold());
    monitor.sendTo(clientId, msg);
}
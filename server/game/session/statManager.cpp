#include "statManager.h"

void StatManager::sendPlayerStats(uint32_t clientId, GameWorld &world,
                                  Monitor &monitor) {
  try {
    const Player &p = world.getPlayer(clientId);

    auto msg = std::make_shared<const PlayerStatsMessage>(
        p.getLevel(), p.getHp(), p.getMaxHp(), p.getMana(), p.getMaxMana(),
        p.getExp(), formulas.calcExpLimit(p.getLevel()), p.getGold());

    monitor.sendTo(clientId, msg);
  } catch (...) {
    throw std::runtime_error("Failed to send player stats");
  }
}
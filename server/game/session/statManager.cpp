#include "statManager.h"

StatManager::StatManager(const toml::table &config)
    : formulas(config) {}

void StatManager::sendPlayerStats(uint32_t clientId,
                                  GameWorld &world,
                                  Monitor &monitor)
{
  const Player &player = world.getPlayer(clientId);
  monitor.sendTo(clientId,
                 std::make_shared<const PlayerStatsMessage>(
                     player.getLevel(),
                     player.getHp(), player.getMaxHp(),
                     player.getMana(), player.getMaxMana(),
                     player.getExp(),
                     formulas.calcExpLimit(player.getLevel()),
                     player.getGold()));
}
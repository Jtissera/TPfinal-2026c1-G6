#include "statManager.h"
#include "statManager.h"

StatManager::StatManager(const toml::table &config)
    : formulas(config)
{
}
void StatManager::sendPlayerStats(uint32_t clientId, GameWorld &world,
                                  Monitor &monitor)
{
  try
  {
    const Player &p = world.getPlayer(clientId);

    uint32_t limit = formulas.calcExpLimit(p.getLevel());

    auto msg = std::make_shared<const PlayerStatsMessage>(
        p.getLevel(), p.getHp(), p.getMaxHp(), p.getMana(), p.getMaxMana(),
        p.getExp(), formulas.calcExpLimit(p.getLevel()), p.getGold());

    monitor.sendTo(clientId, msg);
  }
  catch (...)
  {
    throw std::runtime_error("Failed to send player stats");
  }
}
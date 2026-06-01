#include "playerFactory.h"

PlayerFactory::PlayerFactory(const ClassRepository &classRepo,
                             const RaceRepository &raceRepo,
                             const toml::table &config)
    : classRepo(classRepo), raceRepo(raceRepo), config(config) {}

Player PlayerFactory::create(uint32_t clientId, const std::string &name,
                             const std::string &raceName,
                             const std::string &className, int spawnTileX,
                             int spawnTileY) const {
  const RaceStats &raceStats = raceRepo.get(raceName);
  const ClassStats &classStats = classRepo.get(className);

  int16_t maxHp = formulas.calcMaxHp(raceStats, classStats, 1);
  int16_t maxMana = formulas.calcMaxMana(raceStats, classStats, 1);

  Player player(clientId, name, raceStats, classStats, maxHp, maxMana, config);
  player.setTilePos(spawnTileX, spawnTileY);
  return player;
}
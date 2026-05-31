#include "playerFactory.h"

PlayerFactory::PlayerFactory(const ClassRepository& classRepo,
                             const RaceRepository&  raceRepo)
    : classRepo(classRepo), raceRepo(raceRepo)
{}

Player PlayerFactory::create(uint32_t clientId,
                             const std::string& name,
                             const std::string& raceName,
                             const std::string& className,
                             int spawnTileX,
                             int spawnTileY) const {
    const RaceStats&  raceStats  = raceRepo.get(raceName);
    const ClassStats& classStats = classRepo.get(className);

    int16_t maxHp   = computeMaxHp(raceStats, classStats);
    int16_t maxMana = computeMaxMana(raceStats, classStats);

    Player player(clientId, name, raceStats, classStats, maxHp, maxMana);
    player.setTilePos(spawnTileX, spawnTileY);
    return player;
}

//formulas repetidas

int16_t PlayerFactory::computeMaxHp(const RaceStats& r, const ClassStats& c) {
    return static_cast<int16_t>(100.0f * r.health * c.health);
}

int16_t PlayerFactory::computeMaxMana(const RaceStats& r, const ClassStats& c) {
    return static_cast<int16_t>(100.0f * r.mana * c.mana);
}
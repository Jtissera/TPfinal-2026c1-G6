#include "playerFactory.h"

PlayerFactory::PlayerFactory(const ClassRepository &classRepo,
                             const RaceRepository &raceRepo,
                             const toml::table &config)
    : classRepo(classRepo), raceRepo(raceRepo), config(config) {}

Player PlayerFactory::create(uint32_t clientId,
                              const std::string& name,
                              const std::string& raceName,
                              const std::string& className,
                              int spawnTileX,
                              int spawnTileY) const
{
    const RaceStats&  race = raceRepo.get(raceName);
    const ClassStats& cls  = classRepo.get(className);
 
    int16_t maxHp   = formulas.calcMaxHp(race, cls, 1);
    int16_t maxMana = formulas.calcMaxMana(race, cls, 1);
 
    Player player(clientId, name, race, cls, maxHp, maxMana, config);
 
    int   tileSize = config["world"]["tile_size"].value_or(96);
    float px = static_cast<float>(spawnTileX * tileSize + tileSize / 2);
    float py = static_cast<float>(spawnTileY * tileSize + tileSize / 2);
    player.setPixelPos(px, py);
 
    return player;
}
 
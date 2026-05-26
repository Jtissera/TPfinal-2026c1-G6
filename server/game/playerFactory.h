#pragma once
#include "Player.h"
#include "classRepository.h"
#include "raceRepository.h"
#include "../../common/dtos/gameTypes.h"
#include <string>
#include <cstdint>

class PlayerFactory {
public:

    PlayerFactory(const ClassRepository& classRepo,const RaceRepository&  raceRepo);

    Player create(uint32_t clientId,
                  const std::string& name,
                  const std::string& raceName,
                  const std::string& className,
                  int spawnX,
                  int spawnY) const;

private:
    const ClassRepository& classRepo;
    const RaceRepository&  raceRepo;

    static int16_t computeMaxHp(const RaceStats& r, const ClassStats& c);
    static int16_t computeMaxMana(const RaceStats& r, const ClassStats& c);
};
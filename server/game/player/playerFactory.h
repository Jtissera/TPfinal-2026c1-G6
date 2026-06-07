#pragma once
#include "../stats/classRepository.h"
#include "../stats/gameFormulas.h"
#include "../stats/raceRepository.h"
#include "Player.h"
#include <cstdint>
#include <string>

class PlayerFactory {
public:
  PlayerFactory(const ClassRepository &classRepo,
                const RaceRepository &raceRepo,
                const toml::table &config);

  Player create(uint32_t clientId, const std::string &name,
                const std::string &raceName, const std::string &className,
                int spawnTileX, int spawnTileY) const;

private:
  const ClassRepository &classRepo;
  const RaceRepository &raceRepo;
  const toml::table &config;
  GameFormulas formulas;
};
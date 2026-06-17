#pragma once

#include "classRepository.h"
#include "raceRepository.h"
#include <cstdint>
#include <toml++/toml.hpp>
#include "editor/map/tile.h"

class GameFormulas {
public:
    explicit GameFormulas(const toml::table& config);

    int16_t calcMaxHp(const RaceStats& race, const ClassStats& cls,uint8_t level) const;

    int16_t calcMaxMana(const RaceStats& race, const ClassStats& cls,uint8_t level) const;

    uint32_t calcMaxGold(uint8_t level) const;

    uint32_t calcExpLimit(uint8_t level) const;
    uint32_t calcExpOnHit(int16_t damage, uint8_t attackerLevel,uint8_t targetLevel) const;
    uint32_t calcExpOnKill(int16_t targetMaxHp, uint8_t attackerLevel,uint8_t targetLevel) const;

    uint32_t calcNpcGoldDrop(int16_t npcMaxHp) const;
    uint32_t calcExcessGold(uint32_t gold, uint32_t maxGold) const;
    std::string rollNpcDrop(ZoneType zone) const;

    float calcHpRegen(const RaceStats& race, float deltaSeconds) const;
    float calcManaRegen(const RaceStats& race, float deltaSeconds) const;
    float calcManaRegenMeditating(const ClassStats& cls, const RaceStats& race,
                                  float deltaSeconds) const;

private:
    const toml::table& config;
};

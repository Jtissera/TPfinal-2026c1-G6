#pragma once

#include <cstdint>
#include <string>

#include "../../../common/game/position.h"
#include "../../../common/game/direction.h"
#include "../../game/inventory/inventory.h"
#include "../repositories/raceRepository.h"
#include "../repositories/classRepository.h"
#include "playerState.h"

class Player {
public:
    Player(uint32_t clientId,
           std::string name,
           const RaceStats& race,
           const ClassStats& cls);

    bool isAlive() const;
    bool isGhost() const;
    bool isMeditating() const;

    void takeDamage(int16_t dmg);
    void heal(int16_t amount);
    void restoreMana(int16_t amount);

    void addExperience(uint32_t exp);
    void levelUp(int16_t newMaxHp, int16_t newMaxMana);

    void startMeditating();
    void stopMeditating();

    void die();
    void resurrect(Position nearHealer);

    void tick(float deltaSeconds);

    bool spendMana(int16_t cost);


private:

    uint32_t clientId;
    std::string name;
    const RaceStats& race;
    const ClassStats& cls;

    uint8_t level = 1;

    Position pos;

    int16_t hp = 0;
    int16_t maxHp = 0;
    int16_t mana = 0;
    int16_t maxMana = 0;
    int gold = 0;
    int experience = 0;

    PlayerState state = PlayerState::ALIVE;

    Inventory inventory;
};
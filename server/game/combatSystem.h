#pragma once

#include "../game/Player.h"
#include "gameFormulas.h"

class CombatSystem {
public:
    struct Result {
        bool valid = false;
        bool dodged = false;
        bool killed = false;
        int16_t damage = 0;
        int16_t defense = 0;
        uint32_t expGained = 0;
        uint32_t killExp = 0;
        bool critical = false;
    };

    Result attack(Player& attacker, Player& target);

private:
    GameFormulas formulas;

    bool canAttack(Player& attacker, Player& target);
    bool rollDodge(Player& target);

    int16_t rollDamage(Player& attacker, bool& outCritical);
    int16_t rollDefense(Player& target);

    int16_t rollWeaponDamage(const Item& weapon, bool& outCritical);
    int16_t rollArmorDefense(const Item* item);

    const int MELEE_RANGE = 96;
};
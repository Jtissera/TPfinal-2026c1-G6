#pragma once
#include "Player.h"
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <algorithm>

class CombatSystem {
public:
    struct Result {
        bool     valid    = false;
        bool     dodged   = false;
        bool     killed   = false;
        bool     critical = false;
        int16_t  damage   = 0;
        int16_t  defense  = 0;
        uint32_t expGained = 0;
    };

    Result attack(Combatant& attacker, Combatant& target);
    Result attackPlayer(Player& attacker, Player& target);
    Result attackNpc(Player& attacker, Combatant& target);
    bool canAttack(const Combatant& attacker, const Combatant& target) const;
    bool canAttackPlayer(const Player& attacker, const Player& target) const;

private:
    bool    rollDodge(const Combatant& target) const;
    int16_t rollDamage(const Combatant& attacker, bool& outCritical) const;
    int16_t rollDefense(const Combatant& target) const;
    int16_t rollWeaponDamage(const Item& weapon, bool& outCritical) const;
    int16_t rollArmorDefense(uint16_t min, uint16_t max) const;

    static constexpr int MELEE_RANGE = 1; // en tiles
};
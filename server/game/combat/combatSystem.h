#pragma once
#include "../player/Player.h"
#include "../player/combatant.h"
#include <cstdint>
#include <toml++/toml.hpp>

class CombatSystem {
public:
    explicit CombatSystem(const toml::table& config);

    struct Result {
        bool    valid   = false;
        bool    dodged  = false;
        bool    killed  = false;
        bool    critical= false;
        int16_t damage  = 0;
        int16_t defense = 0;
        uint32_t expGained = 0;
    };

    // Ataque generico entre dos Combatants (jugador vs jugador, NPC vs jugador)
    Result attack(Combatant& attacker, Combatant& target);

    // Ataque jugador vs jugador: valida nivel, mana, newbie protection
    Result attackPlayer(Player& attacker, Player& target);

    // Chequeos de rango y restricciones
    bool canAttack(const Combatant& attacker, const Combatant& target) const;
    bool canAttackPlayer(const Player& attacker, const Player& target) const;

private:
    int     maxLevelDiff;
    int     newbieMaxLevel;

    bool    rollDodge(const Combatant& target) const;
    int16_t rollDamage(const Combatant& attacker, bool& outCritical) const;
    int16_t rollDefense(const Combatant& target) const;
    int16_t rollArmorDefense(uint16_t min, uint16_t max) const;

    // Distancia Chebyshev en pixeles entre dos combatants
    float chebyshevDistancePx(const Combatant& a, const Combatant& b) const;
};
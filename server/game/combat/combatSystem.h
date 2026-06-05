#pragma once
#include "../player/Player.h"
#include "../player/combatant.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <toml++/toml.hpp>

class CombatSystem {
public:
  explicit CombatSystem(const toml::table &config);

  struct Result {
    bool valid = false;
    bool dodged = false;
    bool killed = false;
    bool critical = false;
    int16_t damage = 0;
    int16_t defense = 0;
    uint32_t expGained = 0;
  };

  Result attack(Combatant& attacker, Combatant& target);
  Result attackPlayer(Player& attacker, Player& target);
  Result attackNpc(Player& attacker, Combatant& target);
  bool canAttack(const Combatant &attacker, const Combatant &target) const;
  bool canAttackPlayer(const Player &attacker, const Player &target) const;

private:
  int meleeRange;
  int maxLevelDiff;
  int newbieMaxLevel;

  bool rollDodge(const Combatant &target) const;
  int16_t rollDamage(const Combatant &attacker, bool &outCritical) const;
  int16_t rollDefense(const Combatant &target) const;
  int16_t rollWeaponDamage(const Item &weapon, bool &outCritical) const;
  int16_t rollArmorDefense(uint16_t min, uint16_t max) const;
  static constexpr int MELEE_RANGE = 1; // en tiles
};
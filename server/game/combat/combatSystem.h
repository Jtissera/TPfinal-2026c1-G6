#pragma once
#include "../player/Player.h"
#include "../player/combatant.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <toml++/toml.hpp>

class GameWorld;

class CombatSystem
{
public:
  explicit CombatSystem(const toml::table &config);

  struct Result
  {
    enum class FailReason
    {
      NONE,
      FRIENDLY_FIRE,
      NO_MANA,
      OUT_OF_RANGE,
      LEVEL_TOO_LOW,
      LEVEL_DIFF_TOO_HIGH
    };

    bool valid = false;
    bool dodged = false;
    bool killed = false;
    bool critical = false;
    int16_t damage = 0;
    int16_t defense = 0;
    uint32_t expGained = 0;
    FailReason failReason = FailReason::NONE;
  };

  Result attack(Combatant &attacker, Combatant &target,
                int attackerAllies = 0, int targetAllies = 0);
  Result attackPlayer(Player &attacker, Player &target, const GameWorld &world);
  Result attackNpc(Player &attacker, Combatant &target, const GameWorld &world);
  bool canAttack(const Combatant &attacker, const Combatant &target) const;
  bool canAttackPlayer(const Player &attacker, const Player &target, Result::FailReason &failReason) const;

private:
  int meleeRange;
  int maxLevelDiff;
  int newbieMaxLevel;

  int clanProximityRadius;
  float clanBonusPerAlly;
  float clanMaxBonusMultiplier;

  bool rollDodge(const Combatant &target) const;
  int16_t rollDamage(const Combatant &attacker, bool &outCritical, int nearbyAllies) const;
  int16_t rollDefense(const Combatant &target, int nearbyAllies) const;
  int16_t rollWeaponDamage(const Item &weapon, bool &outCritical) const;
  int16_t rollArmorDefense(uint16_t min, uint16_t max) const;
  float clanMultiplier(int nearbyAllies) const;
  static constexpr int MELEE_RANGE = 1; // en tiles
};
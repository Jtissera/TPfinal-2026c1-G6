#pragma once

#include "../../world/gameWorld.h"
#include "../combat/combatSystem.h"
#include "../combat/itemEffectHandler.h"
#include "../stats/gameFormulas.h"

class CombatHandler
{
public:
  struct Result
  {
    bool valid = false;
    bool attackerLeveledUp = false;
    bool targetDied = false;
    bool statsChanged = false;
  };

  CombatHandler(CombatSystem &combat, ItemEffectHandler &effects,
                GameFormulas &formulas);

  Result handle(uint32_t attackerId, uint32_t targetId, GameWorld &world);

private:
  CombatSystem &combat;
  ItemEffectHandler &effects;
  GameFormulas &formulas;

  Result handleHealWeapon(Player &attacker, Player &target, const Item &weapon);
  Result handleDamageAttack(uint32_t attackerId, Player &attacker,
                            Player &target, GameWorld &world);
};
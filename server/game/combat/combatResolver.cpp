#include "combatResolver.h"

CombatResolver::CombatResolver(CombatSystem &combat,
                               ItemEffectHandler &effects,
                               GameFormulas &formulas)
    : combat(combat), effects(effects), formulas(formulas) {}

CombatResolver::Result CombatResolver::handle(uint32_t attackerId,
                                              uint32_t targetId,
                                              GameWorld &world)
{
  Player &attacker = world.getPlayer(attackerId);
  Player &target = world.getPlayer(targetId);

  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
  if (weapon && weapon->effect == ItemEffect::HEAL)
    return handleHealWeapon(attacker, target, *weapon);

  return handleDamageAttack(attackerId, attacker, target, world);
}

CombatResolver::Result CombatResolver::handleHealWeapon(Player &attacker,
                                                        Player &target,
                                                        const Item &weapon)
{
  Result result;
  result.valid = effects.apply(weapon, attacker, &target);
  result.statsChanged = result.valid;
  return result;
}

CombatResolver::Result CombatResolver::handleDamageAttack(uint32_t attackerId,
                                                          Player &attacker,
                                                          Player &target,
                                                          GameWorld &world)
{
  Result result;

  CombatSystem::Result combatResult = combat.attackPlayer(attacker, target, world);
  if (!combatResult.valid)
    return result;

  result.valid = true;
  result.statsChanged = true;

  if (combatResult.dodged)
    return result;

  world.giveExperience(attackerId, combatResult.expGained);
  result.attackerLeveledUp = attacker.checkAndClearLevelUp();

  if (combatResult.killed)
  {
    world.handlePlayerDeath(target.getId(), attackerId);
    result.targetDied = true;
    if (attacker.checkAndClearLevelUp())
      result.attackerLeveledUp = true;
  }

  return result;
}
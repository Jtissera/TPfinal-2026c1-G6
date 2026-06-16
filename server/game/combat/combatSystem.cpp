#include "combatSystem.h"
#include "../../world/gameWorld.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../clan/clanManager.h"

CombatSystem::CombatSystem(const toml::table &config)
    : meleeRange(config["combat"]["attack_range"].value_or(1)),
      maxLevelDiff(config["combat"]["max_level_diff"].value_or(10)),
      newbieMaxLevel(config["player"]["newbie_max_level"].value_or(12)),
      clanProximityRadius(config["clan"]["proximity_radius_tiles"].value_or(15)),
      clanBonusPerAlly(config["clan"]["bonus_per_ally"].value_or(0.05f)),
      clanMaxBonusMultiplier(config["clan"]["max_bonus_multiplier"].value_or(2.0f)) {}

float CombatSystem::clanMultiplier(int nearbyAllies) const
{
  return std::min(1.0f + nearbyAllies * clanBonusPerAlly, clanMaxBonusMultiplier);
}

CombatSystem::Result CombatSystem::attack(Combatant &attacker, Combatant &target,
                                          int attackerAllies, int targetAllies)
{
  Result result;

  if (!canAttack(attacker, target))
    return result;

  result.valid = true;

  bool critical = false;
  int16_t damage = rollDamage(attacker, critical, attackerAllies);
  result.critical = critical;

  if (!critical && rollDodge(target))
  {
    result.dodged = true;
    return result;
  }

  int16_t defense = rollDefense(target, targetAllies);
  int16_t finalDmg = std::max<int16_t>(0, damage - defense);

  result.damage = finalDmg;
  result.defense = defense;

  target.takeDamage(finalDmg);
  result.killed = !target.isAlive();

  int diff = static_cast<int>(target.getLevel()) -
             static_cast<int>(attacker.getLevel()) + 10;
  result.expGained = (diff > 0) ? static_cast<uint32_t>(finalDmg * diff) : 0;

  return result;
}

CombatSystem::Result CombatSystem::attackPlayer(Player &attacker, Player &target,
                                                const GameWorld &world)
{
  Result result;

  if (!canAttackPlayer(attacker, target))
    return result;

  // Gasto de mana: solo aplica a jugadores con arma equipada que lo requiera
  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
  if (weapon && weapon->stats.manaCost > 0)
    if (!attacker.spendMana(weapon->stats.manaCost))
      return result;

  const int attackerAllies = world.countClanAlliesNear(attacker, clanProximityRadius);
  const int targetAllies = world.countClanAlliesNear(target, clanProximityRadius);

  return attack(attacker, target, attackerAllies, targetAllies);
}

bool CombatSystem::canAttack(const Combatant &attacker, const Combatant &target) const
{
  if (!attacker.isAlive())
    return false;
  if (!target.isAlive())
    return false;

  int dx = std::abs(attacker.getTileX() - target.getTileX());
  int dy = std::abs(attacker.getTileY() - target.getTileY());

  int range = attacker.getAttackRange();
  if (dx > range || dy > range)
    return false;

  return true;
}

bool CombatSystem::canAttackPlayer(const Player &attacker, const Player &target) const
{
  if (!canAttack(attacker, target))
    return false;

  auto attackerClan = ClanManager::instance().findClanInfoForMember(attacker.getName());
  auto targetClan = ClanManager::instance().findClanInfoForMember(target.getName());

  // Jugadores del mismo clan no pueden atacarse entre sí.
  if (attackerClan && targetClan && attackerClan->first == targetClan->first)
  {
    return false;
  }

  if (attacker.getLevel() <= newbieMaxLevel || target.getLevel() <= newbieMaxLevel)
    return false;
  int levelDiff = std::abs((int)attacker.getLevel() - (int)target.getLevel());

  if (levelDiff > maxLevelDiff)
    return false;
  return true;
}

bool CombatSystem::rollDodge(const Combatant &target) const
{
  float agility = static_cast<float>(target.getAgility());
  float roll = (std::rand() % 1000) / 1000.0f;
  return std::pow(roll, agility) < 0.001f;
}

int16_t CombatSystem::rollDamage(const Combatant &attacker,
                                 bool &outCritical, int nearbyAllies) const
{
  int range = attacker.getWeaponDamageMax() - attacker.getWeaponDamageMin();
  int16_t base = static_cast<int16_t>(attacker.getWeaponDamageMin() +
                                      (range > 0 ? std::rand() % range : 0));

  outCritical = (std::rand() % 100) < 5;
  if (outCritical)
    base *= 2;

  return static_cast<int16_t>(base * clanMultiplier(nearbyAllies));
}

int16_t CombatSystem::rollDefense(const Combatant &target, int nearbyAllies) const
{
  int16_t base = rollArmorDefense(target.getArmorDefenseMin(),
                                  target.getArmorDefenseMax()) +
                 rollArmorDefense(target.getHelmetDefenseMin(),
                                  target.getHelmetDefenseMax()) +
                 rollArmorDefense(target.getShieldDefenseMin(),
                                  target.getShieldDefenseMax());

  return static_cast<int16_t>(base * clanMultiplier(nearbyAllies));
}

int16_t CombatSystem::rollArmorDefense(uint16_t min, uint16_t max) const
{
  int range = max - min;
  return static_cast<int16_t>(min + (range > 0 ? std::rand() % range : 0));
}

CombatSystem::Result CombatSystem::attackNpc(Player &attacker, Combatant &target,
                                             const GameWorld &world)
{
  Result result;

  if (!canAttack(attacker, target))
  {
    return result;
  }

  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

  if (weapon != nullptr && weapon->stats.manaCost > 0)
  {
    const int16_t manaCost = static_cast<int16_t>(weapon->stats.manaCost);

    if (!attacker.spendMana(manaCost))
    {
      return result;
    }
  }

  const int attackerAllies = world.countClanAlliesNear(attacker, clanProximityRadius);

  return attack(attacker, target, attackerAllies, 0);
}
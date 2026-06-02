#include "combatSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

CombatSystem::CombatSystem(const toml::table& config)
    : maxLevelDiff(config["combat"]["max_level_diff"].value_or(10))
    , newbieMaxLevel(config["player"]["newbie_max_level"].value_or(12))
{}

float CombatSystem::chebyshevDistancePx(const Combatant& a,
                                         const Combatant& b) const {
    float dx = std::abs(a.getPixelX() - b.getPixelX());
    float dy = std::abs(a.getPixelY() - b.getPixelY());
    return std::max(dx, dy);
}

bool CombatSystem::canAttack(const Combatant& attacker,
                              const Combatant& target) const {
    if (!attacker.isAlive()) return false;
    if (!target.isAlive())   return false;

    // getAttackRangePx() ya distingue melee vs ranged internamente
    float dist = chebyshevDistancePx(attacker, target);
    return dist <= attacker.getAttackRangePx();
}

bool CombatSystem::canAttackPlayer(const Player& attacker,
                                    const Player& target) const {
    if (!canAttack(attacker, target)) return false;
    if (attacker.getLevel() <= newbieMaxLevel ||
        target.getLevel()   <= newbieMaxLevel)
        return false;
    int levelDiff = std::abs(static_cast<int>(attacker.getLevel()) -
                             static_cast<int>(target.getLevel()));
    return levelDiff <= maxLevelDiff;
}

CombatSystem::Result CombatSystem::attack(Combatant& attacker,
                                           Combatant& target) {
    Result result;
    if (!canAttack(attacker, target)) return result;

    result.valid = true;

    bool critical = false;
    int16_t damage = rollDamage(attacker, critical);
    result.critical = critical;

    if (!critical && rollDodge(target)) {
        result.dodged = true;
        return result;
    }

    int16_t defense  = rollDefense(target);
    int16_t finalDmg = std::max<int16_t>(0, damage - defense);

    result.damage  = finalDmg;
    result.defense = defense;

    target.takeDamage(finalDmg);
    result.killed = !target.isAlive();

    int diff = static_cast<int>(target.getLevel()) -
               static_cast<int>(attacker.getLevel()) + 10;
    result.expGained = (diff > 0)
        ? static_cast<uint32_t>(finalDmg * diff)
        : 0;

    return result;
}

CombatSystem::Result CombatSystem::attackPlayer(Player& attacker,
                                                 Player& target) {
    Result result;
    if (!canAttackPlayer(attacker, target)) return result;

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    if (weapon && weapon->stats.manaCost > 0)
        if (!attacker.spendMana(weapon->stats.manaCost))
            return result;

    return attack(attacker, target);
}

bool CombatSystem::rollDodge(const Combatant& target) const {
    float agility = static_cast<float>(target.getAgility());
    float roll    = (std::rand() % 1000) / 1000.0f;
    return std::pow(roll, agility) < 0.001f;
}

int16_t CombatSystem::rollDamage(const Combatant& attacker,
                                  bool& outCritical) const {
    int range = attacker.getWeaponDamageMax() - attacker.getWeaponDamageMin();
    int16_t base = static_cast<int16_t>(
        attacker.getWeaponDamageMin() +
        (range > 0 ? std::rand() % range : 0));

    outCritical = (std::rand() % 100) < 5;
    if (outCritical) base *= 2;
    return base;
}

int16_t CombatSystem::rollDefense(const Combatant& target) const {
    return rollArmorDefense(target.getArmorDefenseMin(),  target.getArmorDefenseMax())
         + rollArmorDefense(target.getHelmetDefenseMin(), target.getHelmetDefenseMax())
         + rollArmorDefense(target.getShieldDefenseMin(), target.getShieldDefenseMax());
}

int16_t CombatSystem::rollArmorDefense(uint16_t min, uint16_t max) const {
    int range = max - min;
    return static_cast<int16_t>(min + (range > 0 ? std::rand() % range : 0));
}
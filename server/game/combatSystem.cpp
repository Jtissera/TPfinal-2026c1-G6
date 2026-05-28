#include "combatSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

CombatSystem::Result CombatSystem::attack(Player& attacker, Player& target) {
    Result result;
    bool critical = false;

    if (!canAttack(attacker, target))
        return result;

    int16_t damage = rollDamage(attacker, critical);
    result.critical = critical;

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    if (weapon && weapon->stats.manaCost > 0) {
        if (!attacker.spendMana(weapon->stats.manaCost))
            return result;
    }

    result.valid = true;

    if (!critical && rollDodge(target)) {
        result.dodged = true;
        return result;
    }

    int16_t defense = rollDefense(target);
    int16_t finalDmg = std::max<int16_t>(0, damage - defense);

    result.damage = finalDmg;
    result.defense = defense;

    target.takeDamage(finalDmg);

    uint32_t exp = formulas.calcExpOnHit(
        finalDmg,
        attacker.getLevel(),
        target.getLevel()
    );

    attacker.addExperience(exp);
    result.expGained = exp;

    if (!target.isAlive()) {
        result.killed = true;

        uint32_t killExp = formulas.calcExpOnKill(
            target.getMaxHp(),
            attacker.getLevel(),
            target.getLevel()
        );

        attacker.addExperience(killExp);
        result.killExp = killExp;
    }

    return result;
}

bool CombatSystem::canAttack(Player& attacker, Player& target) {
    if (!attacker.isAlive()) return false;
    if (!target.isAlive()) return false;

    if (attacker.getLevel() <= 12 || target.getLevel() <= 12)
        return false;

    int levelDiff = std::abs((int)attacker.getLevel() - (int)target.getLevel());
    if (levelDiff > 10) return false;

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    bool isRanged = weapon && weapon->stats.isRanged;

    if (!isRanged) {
        int dx = std::abs(attacker.getX() - target.getX());
        int dy = std::abs(attacker.getY() - target.getY());
        if (dx > MELEE_RANGE || dy > MELEE_RANGE)
            return false;
    }

    return true;
}

bool CombatSystem::rollDodge(Player& target) {
    float agility = static_cast<float>(target.getRace().agility);
    float roll = (std::rand() % 1000) / 1000.0f;
    return std::pow(roll, agility) < 0.001f;
}

int16_t CombatSystem::rollDamage(Player& attacker, bool& outCritical) {
    const Item* weapon =
        attacker.getInventory().getEquipped(EquipSlot::HAND);

    if (!weapon) {
        outCritical = false;
        return 1;
    }

    return rollWeaponDamage(*weapon, outCritical);
}

int16_t CombatSystem::rollWeaponDamage(const Item& weapon, bool& outCritical) {
    int range = weapon.stats.damageMax - weapon.stats.damageMin;

    int16_t base =
        weapon.stats.damageMin +
        (range > 0 ? std::rand() % range : 0);

    outCritical = (std::rand() % 100) < 5;

    if (outCritical)
        base *= 2;

    return base;
}

int16_t CombatSystem::rollDefense(Player& target) {
    const Inventory& inv = target.getInventory();

    int16_t defense = 0;

    defense += rollArmorDefense(inv.getEquipped(EquipSlot::ARMOR));
    defense += rollArmorDefense(inv.getEquipped(EquipSlot::HELMET));
    defense += rollArmorDefense(inv.getEquipped(EquipSlot::SHIELD));

    return defense;
}

int16_t CombatSystem::rollArmorDefense(const Item* item) {
    if (!item) return 0;

    int range = item->stats.defenseMax - item->stats.defenseMin;

    return item->stats.defenseMin +
        (range > 0 ? std::rand() % range : 0);
}
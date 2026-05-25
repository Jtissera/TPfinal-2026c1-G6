#include "itemEffectHandler.h"

bool ItemEffectHandler::apply(const Item& item, Player& user, Player* target) {
    if (item.slot == ItemSlot::CONSUMABLE)
        return applyConsumable(item, user);

    if (item.effect == ItemEffect::HEAL)
        return applyHeal(item, user, target);

    return false;
}

bool ItemEffectHandler::applyConsumable(const Item& item, Player& user) {
    if (!user.isAlive()) return false;

    if (item.stats.healAmount > 0)
        user.heal(item.stats.healAmount);

    if (item.stats.manaAmount > 0)
        user.restoreMana(item.stats.manaAmount);

    return true;
}

bool ItemEffectHandler::applyStaffHeal(const Item& item, Player& user, Player* target) {
    if (!user.isAlive())   return false;
    if (target == nullptr) return false;
    
    if (!target->isAlive()) return false;

    if (!user.spendMana(item.stats.manaCost)) return false;

    target->heal(item.stats.healAmount);
    return true;
}
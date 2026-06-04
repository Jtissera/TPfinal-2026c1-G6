#include "itemEffectHandler.h"

ItemEffectHandler::ItemEffectHandler() {
  handlers[ItemEffect::HEAL] = [this](const Item &item, Player &user,
                                      Player *target) {
    return applyHeal(item, user, target);
  };
}

bool ItemEffectHandler::apply(const Item &item, Player &user, Player *target) {
  if (item.slot == ItemSlot::CONSUMABLE)
    return applyConsumable(item, user);

  auto it = handlers.find(item.effect);
  if (it == handlers.end())
    return false;
  return it->second(item, user, target);
}

bool ItemEffectHandler::applyConsumable(const Item &item, Player &user) {
  if (!user.isAlive())
    return false;

  if (item.stats.healAmount > 0)
    user.heal(item.stats.healAmount);

  if (item.stats.manaAmount > 0)
    user.restoreMana(item.stats.manaAmount);

  return true;
}

bool ItemEffectHandler::applyHeal(const Item &item, Player &user,
                                  Player *target) {
  if (!user.isAlive())
    return false;
  if (!user.spendMana(item.stats.manaCost))
    return false;

  Player *recipient = (target && target->isAlive()) ? target : &user;
  recipient->heal(item.stats.healAmount);
  return true;
}
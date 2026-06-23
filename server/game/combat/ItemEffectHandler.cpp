#include <iostream>

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
  // Si el usuario está muerto, no puede usar la flauta.
  if (!user.isAlive()) {
    std::cout << "[HEAL DEBUG] user muerto, no cura" << std::endl;
    return false;
  }

  // Elegimos a quién se va a curar.
  // Si hay target vivo, cura al target.
  // Si no hay target válido, se cura a sí mismo.
  Player *recipient = (target && target->isAlive()) ? target : &user;

  const int hpBefore = recipient->getHp();
  const int manaBefore = user.getMana();

  std::cout << "[HEAL DEBUG] item='"
            << item.catalogId
            << "' healAmount="
            << item.stats.healAmount
            << " manaCost="
            << item.stats.manaCost
            << " userId="
            << user.getId()
            << " targetId="
            << recipient->getId()
            << " hpBefore="
            << hpBefore
            << " manaBefore="
            << manaBefore
            << std::endl;

  // Primero gastamos maná.
  if (!user.spendMana(item.stats.manaCost)) {
    std::cout << "[HEAL DEBUG] mana insuficiente" << std::endl;
    return false;
  }

  // Aplicamos curación.
  recipient->heal(item.stats.healAmount);

  std::cout << "[HEAL DEBUG] hpAfter="
            << recipient->getHp()
            << " manaAfter="
            << user.getMana()
            << std::endl;

  return true;
}
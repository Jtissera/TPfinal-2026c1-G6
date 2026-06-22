#pragma once

#include "../items/item.h"
#include "../player/Player.h"

class ItemEffectHandler
{
public:
  ItemEffectHandler() = default;

  bool apply(const Item &item, Player &user, Player *target = nullptr);

private:
  bool applyConsumable(const Item &item, Player &user);
  bool applyHeal(const Item &item, Player &user, Player *target);
};
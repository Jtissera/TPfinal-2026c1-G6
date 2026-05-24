#pragma once

#include "../player/player.h"
#include "../../game/inventory/item.h"

class ItemEffectHandler {
public:
  
    bool apply(const Item& item, Player& user, Player* target = nullptr);

private:
    bool applyConsumable(const Item& item, Player& user);
    bool applyHeal(const Item& item, Player& user, Player* target);
};
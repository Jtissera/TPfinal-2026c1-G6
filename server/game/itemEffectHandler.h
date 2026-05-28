#pragma once
#include "Player.h"
#include "item.h"
#include <unordered_map>
#include <functional>

class ItemEffectHandler {
public:
    ItemEffectHandler();
    bool apply(const Item& item, Player& user, Player* target = nullptr);

private:
    using Handler = std::function<bool(const Item&, Player&, Player*)>;
    std::unordered_map<ItemEffect, Handler> handlers;

    bool applyConsumable(const Item& item, Player& user);
    bool applyHeal(const Item& item, Player& user, Player* target);
};
#include "bankAccount.h"
#include <algorithm>

void BankAccount::depositItem(Item item)
{
    items.push_back(std::move(item));
}

std::optional<Item> BankAccount::withdrawItem(const std::string &typeName)
{
    auto it = std::find_if(items.begin(), items.end(),
                           [&](const Item &i)
                           { return i.typeName == typeName; });
    if (it == items.end())
        return std::nullopt;
    Item found = std::move(*it);
    items.erase(it);
    return found;
}

uint32_t BankAccount::depositGold(uint32_t amount)
{
    gold += amount;
    return gold;
}

uint32_t BankAccount::withdrawGold(uint32_t amount)
{
    uint32_t taken = std::min(amount, gold);
    gold -= taken;
    return taken;
}
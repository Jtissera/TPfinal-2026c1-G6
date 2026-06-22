#include "bankAccount.h"
#include <algorithm>

uint32_t BankAccount::getGold() const
{
    return gold;
}

const std::vector<Item> &BankAccount::getItems() const
{
    return items;
}

void BankAccount::depositItem(Item item)
{
    items.push_back(std::move(item));
}

std::optional<Item> BankAccount::withdrawItem(const std::string &typeName)
{
    for (std::vector<Item>::iterator it = items.begin(); it != items.end(); ++it)
    {
        if (it->typeName == typeName)
        {
            Item found = std::move(*it);
            items.erase(it);
            return found;
        }
    }
    return std::nullopt;
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
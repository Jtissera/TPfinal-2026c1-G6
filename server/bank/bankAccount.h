#pragma once
#include "../game/items/item.h"
#include <vector>
#include <cstdint>
#include <optional>

class BankAccount
{
public:
    void depositItem(Item item);

    std::optional<Item> withdrawItem(const std::string &typeName);

    uint32_t depositGold(uint32_t amount);

    uint32_t withdrawGold(uint32_t amount);

    uint32_t getGold() const { return gold; }
    const std::vector<Item> &getItems() const { return items; }

private:
    std::vector<Item> items;
    uint32_t gold = 0;
};
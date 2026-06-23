#pragma once

#include "../game/items/item.h"

#include <cstdint>
#include <vector>

struct DeathResult
{
    uint32_t excessGold = 0;
    uint32_t goldInstanceId = 0;
    std::vector<Item> droppedItems;
    int tileX = 0;
    int tileY = 0;
};
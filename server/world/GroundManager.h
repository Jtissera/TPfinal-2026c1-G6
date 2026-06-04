#pragma once

#include "../game/items/item.h"
#include <cstdint>
#include <optional>
#include <vector>

class GroundManager {
public:
  void addItem(Item item, int tileX, int tileY);
  std::optional<Item> pickItemAt(int tileX, int tileY);

  void addGold(uint32_t amount, int tileX, int tileY);
  std::optional<uint32_t> pickGoldAt(int tileX, int tileY);

private:
  struct GroundItem {
    Item item;
    int tileX, tileY;
  };
  struct GroundGold {
    uint32_t amount;
    int tileX, tileY;
  };

  std::vector<GroundItem> groundItems;
  std::vector<GroundGold> groundGold;
};
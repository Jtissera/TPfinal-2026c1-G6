#pragma once

#include "../game/items/item.h"
#include <cstdint>
#include <optional>
#include <vector>

struct GroundItem {
  Item item;
  int tileX, tileY;
};
struct GroundGold {
  uint32_t instanceId;
  uint32_t amount;
  int tileX, tileY;
};

class GroundManager {
public:
  void addItem(Item item, int tileX, int tileY);
  std::optional<Item> pickItemById(uint32_t instanceId);

  uint32_t addGold(uint32_t amount, int tileX, int tileY);
  std::optional<uint32_t> pickGoldById(uint32_t instanceId);

  const std::vector<GroundItem>& getAllItems() const;
  const std::vector<GroundGold>& getAllGold() const;


private:


  std::vector<GroundItem> groundItems;
  std::vector<GroundGold> groundGold;

  uint32_t nextGoldInstanceId = 1;
};
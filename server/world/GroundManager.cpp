#include "GroundManager.h"

void GroundManager::addItem(Item item, int tileX, int tileY) {
  groundItems.push_back({std::move(item), tileX, tileY});
}

std::optional<Item> GroundManager::pickItemAt(int tileX, int tileY) {
  for (auto it = groundItems.begin(); it != groundItems.end(); ++it) {
    if (it->tileX == tileX && it->tileY == tileY) {
      Item found = std::move(it->item);
      groundItems.erase(it);
      return found;
    }
  }
  return std::nullopt;
}

void GroundManager::addGold(uint32_t amount, int tileX, int tileY) {
  groundGold.push_back({amount, tileX, tileY});
}

std::optional<uint32_t> GroundManager::pickGoldAt(int tileX, int tileY) {
  for (auto it = groundGold.begin(); it != groundGold.end(); ++it) {
    if (it->tileX == tileX && it->tileY == tileY) {
      uint32_t amount = it->amount;
      groundGold.erase(it);
      return amount;
    }
  }
  return std::nullopt;
}
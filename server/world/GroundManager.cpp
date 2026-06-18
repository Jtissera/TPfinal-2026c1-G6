#include "GroundManager.h"

void GroundManager::addItem(Item item, int tileX, int tileY) {
  groundItems.push_back({std::move(item), tileX, tileY});
}

std::optional<Item> GroundManager::pickItemById(uint32_t instanceId) {
  for (auto it = groundItems.begin(); it != groundItems.end(); ++it) {
    if (it->item.instanceId == instanceId) {
      Item found = std::move(it->item);
      groundItems.erase(it);
      return found;
    }
  }
  return std::nullopt;
}


uint32_t GroundManager::addGold(uint32_t amount, int tileX, int tileY) {
  const uint32_t instanceId = nextGoldInstanceId++;
  groundGold.push_back({instanceId,amount,tileX,tileY});
  return instanceId;
}

std::optional<uint32_t> GroundManager::pickGoldById(uint32_t instanceId) {
  for (auto it = groundGold.begin(); it != groundGold.end(); ++it) {
    if (it->instanceId == instanceId) {
      uint32_t amount = it->amount;
      groundGold.erase(it);
      return amount;
    }
  }
  return std::nullopt;
}

const std::vector<GroundItem> & GroundManager::getAllItems() const {
  return groundItems;
}

const std::vector<GroundGold> & GroundManager::getAllGold() const {
  return groundGold;
}

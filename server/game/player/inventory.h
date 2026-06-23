#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "server/game/items/EquipSlot.h"
#include "server/game/items/item.h"
#include "server/game/items/itemSlot.h"

class Inventory
{
public:
  static const uint32_t EMPTY_SLOT;
  static const std::size_t MAX_INVENTORY_SLOTS;

  explicit Inventory(std::size_t maxItems);

  bool canAddItem() const;
  bool addItem(Item item);
  bool equipItem(uint32_t itemId);
  bool unequipSlot(EquipSlot slot);
  std::optional<Item> removeItem(uint32_t itemId);
  std::optional<Item> removeItemByName(const std::string &typeName);

  const Item *getEquipped(EquipSlot slot) const;
  const std::vector<Item> &getItems() const;
  const std::array<uint32_t, 20> &getInventorySlots() const;
  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &getEquippedArray();

  std::vector<Item> removeAllItems();
  Item *findItem(uint32_t itemId);
  const Item *findItem(uint32_t itemId) const;
  bool hasItem(const std::string &name) const;

private:
  std::size_t maxItems;
  std::vector<Item> items;
  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};
  std::array<uint32_t, 20> inventorySlots{};

  std::optional<std::size_t> findFirstFreeInventorySlot() const;
  void removeFromInventorySlots(uint32_t itemId);
};
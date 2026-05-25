#include "inventory.h"
#include <algorithm>

std::optional<EquipSlot> toEquipSlot(ItemSlot slot) {
  switch (slot) {
  case ItemSlot::WEAPON:
  case ItemSlot::STAFF:
    return EquipSlot::HAND;
  case ItemSlot::ARMOR:
    return EquipSlot::ARMOR;
  case ItemSlot::HELMET:
    return EquipSlot::HELMET;
  case ItemSlot::SHIELD:
    return EquipSlot::SHIELD;
  default:
    return std::nullopt;
  }
}

bool Inventory::addItem(Item item) {
  if (items.size() >= MAX_ITEMS)
    return false;
  items.push_back(std::move(item));

  return true;
}

bool Inventory::equipItem(uint32_t itemId) {

  Item *item = findItem(itemId);
  if (!item)
    return false;

  auto slot = toEquipSlot(item->slot);
  if (!slot)
    return false;

  auto idx = static_cast<std::size_t>(*slot);

  equipped[idx] = itemId;
  return true;
}

bool Inventory::unequipSlot(EquipSlot slot) {
  auto idx = static_cast<std::size_t>(slot);

  if (equipped[idx] == EMPTY_SLOT)
    return false;

  equipped[idx] = EMPTY_SLOT;
  return true;
}

std::optional<Item> Inventory::removeItem(uint32_t itemId) {

  for (auto &slot : equipped) {
    if (slot == itemId)
      slot = EMPTY_SLOT;
  }

  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.id == itemId; });

  if (it == items.end())
    return std::nullopt;

  Item removed = std::move(*it);
  items.erase(it);
  return removed;
}

const Item *Inventory::getEquipped(EquipSlot slot) const {
  uint32_t id = equipped[static_cast<std::size_t>(slot)];

  if (id == EMPTY_SLOT)
    return nullptr;

  return findItem(id);
}

const std::vector<Item> &Inventory::getItems() const { return items; }

const Item *Inventory::findItem(uint32_t itemId) const {

  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.id == itemId; });

  if (it != items.end()) {
    return &(*it);
  }

  return nullptr;
}

Item *Inventory::findItem(uint32_t itemId) {
  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.id == itemId; });

  if (it != items.end()) {
    return &(*it);
  }

  return nullptr;
}
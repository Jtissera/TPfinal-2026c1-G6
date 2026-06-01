#include "inventory.h"
#include <algorithm>

Inventory::Inventory(const toml::table &config)
    : maxItems(
          config["player"]["max_inventory_items"].value_or<std::size_t>(20)) {}

std::optional<EquipSlot> toEquipSlot(ItemSlot slot) {
  const std::map<ItemSlot, EquipSlot> mapping = {
      {ItemSlot::WEAPON, EquipSlot::HAND},
      {ItemSlot::STAFF, EquipSlot::HAND},
      {ItemSlot::ARMOR, EquipSlot::ARMOR},
      {ItemSlot::HELMET, EquipSlot::HELMET},
      {ItemSlot::SHIELD, EquipSlot::SHIELD},
  };

  auto it = mapping.find(slot);
  if (it == mapping.end())
    return std::nullopt;
  return it->second;
}

bool Inventory::addItem(Item item) {
  if (items.size() >= maxItems)
    return false;
  items.push_back(std::move(item));
  return true;
}

std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &
Inventory::getEquippedArray() {
  return equipped;
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
  if (equipped[idx] == 0)
    return false;
  equipped[idx] = 0;
  return true;
}

std::optional<Item> Inventory::removeItem(uint32_t itemId) {
  for (auto &slot : equipped)
    if (slot == itemId)
      slot = 0;

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
  if (id == 0)
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

std::vector<Item> Inventory::removeAllItems() {
  for (auto &slot : equipped)
    slot = 0;
  std::vector<Item> all = std::move(items);
  items.clear();
  return all;
}
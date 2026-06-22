#include "inventory.h"

const uint32_t Inventory::EMPTY_SLOT = 0;
const std::size_t Inventory::MAX_INVENTORY_SLOTS = 20;

namespace
{
  std::optional<EquipSlot> toEquipSlot(ItemSlot slot)
  {
    if (slot == ItemSlot::WEAPON)
      return EquipSlot::HAND;
    if (slot == ItemSlot::STAFF)
      return EquipSlot::HAND;
    if (slot == ItemSlot::ARMOR)
      return EquipSlot::ARMOR;
    if (slot == ItemSlot::HELMET)
      return EquipSlot::HELMET;
    if (slot == ItemSlot::SHIELD)
      return EquipSlot::SHIELD;
    return std::nullopt;
  }
}

Inventory::Inventory(std::size_t maxItems)
    : maxItems(maxItems)
{
  equipped.fill(EMPTY_SLOT);
  inventorySlots.fill(EMPTY_SLOT);
}

const std::vector<Item> &Inventory::getItems() const
{
  return items;
}

const std::array<uint32_t, 20> &Inventory::getInventorySlots() const
{
  return inventorySlots;
}

std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &Inventory::getEquippedArray()
{
  return equipped;
}

const Item *Inventory::getEquipped(EquipSlot slot) const
{
  uint32_t id = equipped[static_cast<std::size_t>(slot)];
  if (id == EMPTY_SLOT)
    return nullptr;
  return findItem(id);
}

bool Inventory::canAddItem() const
{
  return findFirstFreeInventorySlot().has_value();
}

bool Inventory::addItem(Item item)
{
  std::optional<std::size_t> slotIdx = findFirstFreeInventorySlot();
  if (!slotIdx)
    return false;

  items.push_back(std::move(item));
  inventorySlots[*slotIdx] = items.back().instanceId;
  return true;
}

std::optional<std::size_t> Inventory::findFirstFreeInventorySlot() const
{
  for (std::size_t i = 0; i < maxItems; ++i)
  {
    if (inventorySlots[i] == EMPTY_SLOT)
      return i;
  }
  return std::nullopt;
}

void Inventory::removeFromInventorySlots(uint32_t itemId)
{
  for (std::size_t i = 0; i < inventorySlots.size(); ++i)
  {
    if (inventorySlots[i] == itemId)
      inventorySlots[i] = EMPTY_SLOT;
  }
}

bool Inventory::equipItem(uint32_t itemId)
{
  Item *item = findItem(itemId);
  if (!item)
    return false;

  std::optional<EquipSlot> slot = toEquipSlot(item->slot);
  if (!slot.has_value())
    return false;

  std::size_t equipIdx = static_cast<std::size_t>(slot.value());
  if (equipIdx >= equipped.size())
    return false;

  uint32_t previousItemId = equipped[equipIdx];
  if (previousItemId != EMPTY_SLOT)
  {
    std::optional<std::size_t> freeSlot = findFirstFreeInventorySlot();
    if (!freeSlot.has_value())
      return false;
    inventorySlots[freeSlot.value()] = previousItemId;
  }

  equipped[equipIdx] = itemId;
  removeFromInventorySlots(itemId);
  return true;
}

bool Inventory::unequipSlot(EquipSlot slot)
{
  std::size_t idx = static_cast<std::size_t>(slot);
  if (idx >= equipped.size())
    return false;

  uint32_t equippedItemId = equipped[idx];
  if (equippedItemId == EMPTY_SLOT)
    return false;

  std::optional<std::size_t> freeSlot = findFirstFreeInventorySlot();
  if (!freeSlot.has_value())
    return false;

  inventorySlots[freeSlot.value()] = equippedItemId;
  equipped[idx] = EMPTY_SLOT;
  return true;
}

std::optional<Item> Inventory::removeItem(uint32_t itemId)
{
  for (std::size_t i = 0; i < equipped.size(); ++i)
    if (equipped[i] == itemId)
      equipped[i] = EMPTY_SLOT;

  removeFromInventorySlots(itemId);

  for (std::vector<Item>::iterator it = items.begin(); it != items.end(); ++it)
  {
    if (it->instanceId == itemId)
    {
      Item removed = std::move(*it);
      items.erase(it);
      return removed;
    }
  }
  return std::nullopt;
}

std::optional<Item> Inventory::removeItemByName(const std::string &typeName)
{
  for (std::vector<Item>::iterator it = items.begin(); it != items.end(); ++it)
  {
    if (it->typeName == typeName)
    {
      for (std::size_t i = 0; i < equipped.size(); ++i)
        if (equipped[i] == it->instanceId)
          equipped[i] = EMPTY_SLOT;

      removeFromInventorySlots(it->instanceId);

      Item found = std::move(*it);
      items.erase(it);
      return found;
    }
  }
  return std::nullopt;
}

Item *Inventory::findItem(uint32_t itemId)
{
  for (std::vector<Item>::iterator it = items.begin(); it != items.end(); ++it)
    if (it->instanceId == itemId)
      return &(*it);
  return nullptr;
}

const Item *Inventory::findItem(uint32_t itemId) const
{
  for (std::vector<Item>::const_iterator it = items.begin(); it != items.end(); ++it)
    if (it->instanceId == itemId)
      return &(*it);
  return nullptr;
}

std::vector<Item> Inventory::removeAllItems()
{
  for (std::size_t i = 0; i < equipped.size(); ++i)
    equipped[i] = EMPTY_SLOT;
  for (std::size_t i = 0; i < inventorySlots.size(); ++i)
    inventorySlots[i] = EMPTY_SLOT;

  std::vector<Item> all = std::move(items);
  items.clear();
  return all;
}

bool Inventory::hasItem(const std::string &name) const
{
  for (std::vector<Item>::const_iterator it = items.begin(); it != items.end(); ++it)
    if (it->typeName == name)
      return true;
  return false;
}
#include "inventory.h"
#include <algorithm>

Inventory::Inventory()
    : maxItems(MAX_INVENTORY_SLOTS) {
  equipped.fill(EMPTY_SLOT);
  inventorySlots.fill(EMPTY_SLOT);
}

Inventory::Inventory(const toml::table& config)
    : maxItems(config["player"]["max_inventory_items"].value_or<std::size_t>(
          static_cast<std::size_t>(MAX_INVENTORY_SLOTS))) {
  equipped.fill(EMPTY_SLOT);
  inventorySlots.fill(EMPTY_SLOT);
}
std::optional<EquipSlot> toEquipSlot(ItemSlot slot)
{
  const std::map<ItemSlot, EquipSlot> mapping = {
      {ItemSlot::WEAPON, EquipSlot::HAND},
      {ItemSlot::STAFF,  EquipSlot::HAND},
      {ItemSlot::ARMOR,  EquipSlot::ARMOR},
      {ItemSlot::HELMET, EquipSlot::HELMET},
      {ItemSlot::SHIELD, EquipSlot::SHIELD},
  };
  auto it = mapping.find(slot);
  if (it == mapping.end())
    return std::nullopt;
  return it->second;
}

bool Inventory::addItem(Item item)
{
  if (items.size() >= MAX_INVENTORY_SLOTS)
    return false;

  // Poner en el primer slot libre del array de slots
  auto slotIdx = findFirstFreeInventorySlot();
  if (!slotIdx)
    return false;

  items.push_back(std::move(item));
  inventorySlots[*slotIdx] = items.back().instanceId;
  return true;
}

std::optional<std::size_t> Inventory::findFirstFreeInventorySlot() const
{
  for (std::size_t i = 0; i < MAX_INVENTORY_SLOTS; ++i)
    if (inventorySlots[i] == EMPTY_SLOT)
      return i;
  return std::nullopt;
}

void Inventory::removeFromInventorySlots(uint32_t itemId)
{
  for (auto &slot : inventorySlots)
    if (slot == itemId)
      slot = EMPTY_SLOT;
}

bool Inventory::equipItem(uint32_t itemId) {
  Item* item = findItem(itemId);

  if (!item) {
    return false;
  }

  const auto slot = toEquipSlot(item->slot);

  if (!slot.has_value()) {
    return false;
  }

  const auto idx = static_cast<std::size_t>(slot.value());

  if (idx >= equipped.size()) {
    return false;
  }

  if (equipped[idx] == itemId) {
    return true;
  }

  const uint32_t previousEquippedId = equipped[idx];

  if (previousEquippedId != EMPTY_SLOT) {
    const auto freeSlot = findFirstFreeInventorySlot();

    if (!freeSlot.has_value()) {
      return false;
    }

    inventorySlots[freeSlot.value()] = previousEquippedId;
  }

  removeFromInventorySlots(itemId);

  equipped[idx] = itemId;

  return true;
}

bool Inventory::unequipSlot(EquipSlot slot) {
  const auto idx = static_cast<std::size_t>(slot);

  if (idx >= equipped.size()) {
    return false;
  }

  const uint32_t equippedItemId = equipped[idx];

  if (equippedItemId == EMPTY_SLOT) {
    return false;
  }

  const auto freeSlot = findFirstFreeInventorySlot();

  if (!freeSlot.has_value()) {
    return false;
  }

  inventorySlots[freeSlot.value()] = equippedItemId;
  equipped[idx] = EMPTY_SLOT;

  return true;
}

std::optional<Item> Inventory::removeItem(uint32_t itemId)
{
  // Sacar de equipped si está equipado
  for (auto &slot : equipped)
    if (slot == itemId)
      slot = EMPTY_SLOT;

  removeFromInventorySlots(itemId);

  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.instanceId == itemId; });
  if (it == items.end())
    return std::nullopt;

  Item removed = std::move(*it);
  items.erase(it);
  return removed;
}

// Usado por bankerHandler y merchantHandler (busca por nombre de tipo)
std::optional<Item> Inventory::removeItemByName(const std::string &typeName)
{
  auto it = std::find_if(items.begin(), items.end(),
                         [&](const Item &i) { return i.typeName == typeName; });
  if (it == items.end())
    return std::nullopt;

  // Limpiar de equipped y slots
  for (auto &slot : equipped)
    if (slot == it->instanceId)
      slot = EMPTY_SLOT;
  removeFromInventorySlots(it->instanceId);

  Item found = std::move(*it);
  items.erase(it);
  return found;
}

const Item *Inventory::getEquipped(EquipSlot slot) const
{
  uint32_t id = equipped[static_cast<std::size_t>(slot)];
  if (id == EMPTY_SLOT)
    return nullptr;
  return findItem(id);
}

const std::vector<Item> &Inventory::getItems() const { return items; }

const std::array<uint32_t, Inventory::MAX_INVENTORY_SLOTS>&
Inventory::getInventorySlots() const { return inventorySlots; }

Item *Inventory::findItem(uint32_t itemId)
{
  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.instanceId == itemId; });
  return it != items.end() ? &(*it) : nullptr;
}

const Item *Inventory::findItem(uint32_t itemId) const
{
  auto it = std::find_if(items.begin(), items.end(),
                         [itemId](const Item &i) { return i.instanceId == itemId; });
  return it != items.end() ? &(*it) : nullptr;
}

std::vector<Item> Inventory::removeAllItems()
{
  for (auto &slot : equipped)
    slot = EMPTY_SLOT;
  for (auto &slot : inventorySlots)
    slot = EMPTY_SLOT;

  std::vector<Item> all = std::move(items);
  items.clear();
  return all;
}


bool Inventory::hasItem(const std::string &name) const
{
  return std::any_of(items.begin(), items.end(),
                     [&](const Item &i)
                     { return i.typeName == name; });
}
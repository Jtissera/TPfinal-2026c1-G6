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
  const auto freeSlot = findFirstFreeInventorySlot();
  if (!freeSlot.has_value()) {
    return false;
  }
  const uint32_t itemId = item.instanceId;
  items.push_back(std::move(item));
  inventorySlots[freeSlot.value()]= itemId;
  return true;
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

std::optional<Item> Inventory::removeItem(uint32_t itemId) {
  // itemId representa instanceId.

  // Si estaba equipado, lo des-equipamos.
  for (auto& slot : equipped) {
    if (slot == itemId) {
      slot = EMPTY_SLOT;
    }
  }

  removeFromInventorySlots(itemId);

  auto it = std::find_if(
      items.begin(),
      items.end(),
      [itemId](const Item& item) {
          return item.instanceId == itemId;
      }
  );

  if (it == items.end()) {
    return std::nullopt;
  }

  Item removed = std::move(*it);
  items.erase(it);

  return removed;
}

const Item *Inventory::getEquipped(EquipSlot slot) const {
  const auto idx = static_cast<std::size_t>(slot);

  if (idx >= equipped.size()) {
    return nullptr;
  }

  const uint32_t itemId = equipped[idx];

  if (itemId == EMPTY_SLOT) {
    return nullptr;
  }

  return findItem(itemId);
}

const std::vector<Item> &Inventory::getItems() const { return items; }

const Item *Inventory::findItem(uint32_t itemId) const {

  auto it = std::find_if(items.begin(), items.end(),[itemId](const Item& item) {
    return item.instanceId == itemId;
  });

  if (it != items.end()) {
    return &(*it);
  }

  return nullptr;
}


Item* Inventory::findItem(uint32_t itemId) {
  // itemId representa instanceId.
  auto it = std::find_if(items.begin(),items.end(),[itemId](const Item& item) {
          return item.instanceId == itemId;
      }
  );

  if (it != items.end()) {
    return &(*it);
  }

  return nullptr;
}

std::vector<Item> Inventory::removeAllItems() {
  for (auto& slot : equipped) {
    slot = EMPTY_SLOT;
  }

  for (auto& slot : inventorySlots) {
    slot = EMPTY_SLOT;
  }

  std::vector<Item> all = std::move(items);
  items.clear();

  return all;
}

const std::array<uint32_t, Inventory::MAX_INVENTORY_SLOTS>& Inventory::getInventorySlots() const {
  return inventorySlots;
}

std::optional<std::size_t> Inventory::findFirstFreeInventorySlot() const {
  for (std::size_t i = 0; i < inventorySlots.size(); ++i) {
    if (inventorySlots[i] == EMPTY_SLOT) {
      return i;
    }
  }

  return std::nullopt;
}

void Inventory::removeFromInventorySlots(uint32_t itemId) {
  for (auto& slot : inventorySlots) {
    if (slot == itemId) {
      slot = EMPTY_SLOT;
    }
  }
}
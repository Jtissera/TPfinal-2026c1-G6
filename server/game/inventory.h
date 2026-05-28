#pragma once

#include "itemRepository.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "EquipSlot.h"

std::optional<EquipSlot> toEquipSlot(ItemSlot slot);

class Inventory {
public:
  // constante a TOML
  static constexpr std::size_t MAX_ITEMS = 20;
  static constexpr uint32_t EMPTY_SLOT = 0;

  bool addItem(Item item);
  bool equipItem(uint32_t itemId);
  bool unequipSlot(EquipSlot slot);
  std::optional<Item> removeItem(uint32_t itemId);

  const Item *getEquipped(EquipSlot slot) const;
  const std::vector<Item> &getItems() const;

  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)>& getEquippedArray(){
    return equipped;
  }

  std::vector<Item> removeAllItems();

private:
  std::vector<Item> items;
  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};

  const Item *findItem(uint32_t itemId) const;
  Item *findItem(uint32_t itemId);
};
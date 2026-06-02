#pragma once

#include "../items/EquipSlot.h"
#include "../items/itemRepository.h"
#include <array>
#include <cstdint>
#include <optional>
#include <toml++/toml.hpp>
#include <vector>

std::optional<EquipSlot> toEquipSlot(ItemSlot slot);

class Inventory
{
public:
  explicit Inventory(const toml::table &config);

  bool addItem(Item item);
  bool equipItem(uint32_t itemId);
  bool unequipSlot(EquipSlot slot);
  bool hasItem(const std::string &name) const;
  std::optional<Item> removeItem(uint32_t itemId);

  const Item *getEquipped(EquipSlot slot) const;
  const std::vector<Item> &getItems() const;

  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &
  getEquippedArray();

  std::vector<Item> removeAllItems();

  Item *findItem(uint32_t itemId);
  const Item *findItem(uint32_t itemId) const;

  std::optional<Item> removeItemByName(const std::string &typeName);

private:
  std::size_t maxItems;
  std::vector<Item> items;
  std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};
};
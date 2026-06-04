
#ifndef TALLER_TP_INVENTORY_H
#define TALLER_TP_INVENTORY_H

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <toml++/toml.h>

#include "server/game/items/EquipSlot.h"
#include "server/game/items/item.h"
#include "server/game/items/itemSlot.h"

std::optional<EquipSlot> toEquipSlot(ItemSlot slot);

class Inventory {
public:
    static constexpr uint32_t EMPTY_SLOT = 0;
    static constexpr std::size_t MAX_INVENTORY_SLOTS = 20;

    // Constructor por defecto para tests o código viejo.
    Inventory();

    // Constructor nuevo compatible con dev/TOML.
    explicit Inventory(const toml::table& config);

    bool addItem(Item item);
    bool equipItem(uint32_t itemId);
    bool unequipSlot(EquipSlot slot);
    std::optional<Item> removeItem(uint32_t itemId);

    const Item* getEquipped(EquipSlot slot) const;
    const std::vector<Item>& getItems() const;

    const std::array<uint32_t, MAX_INVENTORY_SLOTS>& getInventorySlots() const;

    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)>& getEquippedArray() {
        return equipped;
    }

    const std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)>& getEquippedArray() const {
        return equipped;
    }

    std::vector<Item> removeAllItems();

    Item* findItem(uint32_t itemId);
    const Item* findItem(uint32_t itemId) const;

private:
    std::vector<Item> items;

    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};
    std::array<uint32_t, MAX_INVENTORY_SLOTS> inventorySlots{};

    std::size_t maxItems = MAX_INVENTORY_SLOTS;

    std::optional<std::size_t> findFirstFreeInventorySlot() const;
    void removeFromInventorySlots(uint32_t itemId);
};


#endif //TALLER_TP_INVENTORY_H

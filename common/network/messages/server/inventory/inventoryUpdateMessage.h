#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "server/game/items/EquipSlot.h"
#include "server/game/items/item.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class InventoryUpdateMessage : public Message
{
public:
    static constexpr std::size_t INVENTORY_SLOT_COUNT = 20;

    InventoryUpdateMessage(
        std::vector<Item> items,
        std::array<uint32_t, INVENTORY_SLOT_COUNT> inventorySlots,
        std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped);

    const std::vector<Item> &getItems() const;
    const std::array<uint32_t, INVENTORY_SLOT_COUNT> &getInventorySlots() const;
    const std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &getEquipped() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::vector<Item> items;
    std::array<uint32_t, INVENTORY_SLOT_COUNT> inventorySlots;
    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped;
};
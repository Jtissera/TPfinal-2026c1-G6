#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "server/game/items/EquipSlot.h"
#include "server/game/items/item.h"


class InventoryUpdateMessage : public Message {
public:
    // Cantidad de slots visuales/lógicos del inventario.
    // Debe ser public porque el deserializer del cliente la usa.
    static constexpr std::size_t INVENTORY_SLOT_COUNT = 20;

private:
    std::vector<Item> items;

    std::array<uint32_t, INVENTORY_SLOT_COUNT> inventorySlots{};

    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};

public:
    InventoryUpdateMessage(
        std::vector<Item> items,
        std::array<uint32_t, INVENTORY_SLOT_COUNT> inventorySlots,
        std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped
    );

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;

    const std::vector<Item>& getItems() const;

    const std::array<uint32_t, INVENTORY_SLOT_COUNT>& getInventorySlots() const;

    const std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)>& getEquipped() const;
};

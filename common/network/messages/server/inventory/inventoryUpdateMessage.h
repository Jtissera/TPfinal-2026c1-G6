#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/dtos/itemDto.h"
#include "server/game/items/item.h"
#include <array>
#include <vector>

static constexpr std::size_t EQUIP_SLOT_COUNT = 4;

class InventoryUpdateMessage : public Message {
    std::vector<ItemDto> items;
    std::array<uint32_t, EQUIP_SLOT_COUNT> equipped{};

public:
    // Constructor del servidor: recibe vector<Item> y convierte a ItemDto
    InventoryUpdateMessage(
        const std::vector<Item>& serverItems,
        std::array<uint32_t, EQUIP_SLOT_COUNT> equipped)
    : equipped(equipped)
    {
        items.reserve(serverItems.size());
        for (const auto& it : serverItems) {
            ItemDto dto;
            dto.id       = it.id;
            dto.typeName = it.typeName;
            dto.slot     = static_cast<uint8_t>(it.slot);
            items.push_back(std::move(dto));
        }
    }

    // Constructor del cliente deserializador: recibe vector<ItemDto> directo
    InventoryUpdateMessage(
        std::vector<ItemDto> items,
        std::array<uint32_t, EQUIP_SLOT_COUNT> equipped)
    : items(std::move(items)), equipped(equipped) {}

    const std::vector<ItemDto>& getItems()    const { return items;    }
    const auto&                 getEquipped() const { return equipped; }

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_INVENTORY_UPDATE);
    }

    void serializeBody(PacketWriter& writer) const override {
        writer.writeUint8(static_cast<uint8_t>(items.size()));
        for (const auto& item : items) {
            writer.writeUint32(item.id);
            writer.writeString(item.typeName);
            writer.writeUint8(item.slot);
        }
        for (uint32_t equippedId : equipped)
            writer.writeUint32(equippedId);
    }
};
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"

#include "server/game/item.h"
#include "server/game/EquipSlot.h"

class InventoryUpdateMessage : public Message {
    std::vector<Item> items;
    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped;
public:
    InventoryUpdateMessage(std::vector<Item> items,
                           std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped)
        : items(std::move(items))
        , equipped(equipped) {}

    const std::vector<Item>& getItems()    const { return items;    }
    const auto&              getEquipped() const { return equipped;  }

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_INVENTORY_UPDATE);
    }

    void serializeBody(PacketWriter& writer) const  override{
        writer.writeUint8(static_cast<uint8_t>(items.size()));
        for (const Item& item : items) {
            writer.writeUint32(item.instanceId);
            writer.writeUint32(item.catalogId);
            writer.writeString(item.typeName);
            writer.writeUint8(static_cast<uint8_t>(item.slot));
        }
        for (uint32_t equippedId : equipped) {
            writer.writeUint32(equippedId);
        }
    }
};
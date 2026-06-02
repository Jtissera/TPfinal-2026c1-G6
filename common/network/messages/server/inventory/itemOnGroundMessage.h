#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/dtos/itemDto.h"

class ItemOnGroundMessage : public Message {
    ItemDto item;
    int x, y;

public:
    ItemOnGroundMessage(ItemDto item, int x, int y)
        : item(std::move(item)), x(x), y(y) {}

    const ItemDto& getItem() const { return item; }
    int getX() const { return x; }
    int getY() const { return y; }

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_ITEM_ON_GROUND);
    }

    void serializeBody(PacketWriter& writer) const override {
        writer.writeUint32(item.id);
        writer.writeString(item.typeName);
        writer.writeUint16(static_cast<uint16_t>(x));
        writer.writeUint16(static_cast<uint16_t>(y));
    }
};
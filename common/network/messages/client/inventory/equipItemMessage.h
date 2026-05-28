#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"

class EquipItemMessage : public Message {
    uint32_t itemId;
public:
    explicit EquipItemMessage(uint32_t itemId) : itemId(itemId) {}
    uint32_t getItemId() const { return itemId; }
    uint8_t opCode() const override {
        return static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM);
    }

    void serializeBody(PacketWriter& writer) const override {
    writer.writeUint32(itemId);
}
};
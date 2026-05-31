#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"

class ItemPickedMessage : public Message {
    uint32_t clientId;
    uint32_t itemId;
public:
    ItemPickedMessage(uint32_t clientId, uint32_t itemId)
        : clientId(clientId), itemId(itemId) {}

    uint32_t getClientId() const { return clientId; }
    uint32_t getItemId()   const { return itemId;   }

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_ITEM_PICKED);
    }

void serializeBody(PacketWriter& writer) const override{
    writer.writeUint32(clientId);
    writer.writeUint32(itemId);
}
};
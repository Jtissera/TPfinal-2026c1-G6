#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class ItemPickedMessage : public Message
{
public:
    ItemPickedMessage(uint32_t clientId, uint32_t itemId);

    uint32_t getClientId() const;
    uint32_t getItemId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t clientId;
    uint32_t itemId;
};
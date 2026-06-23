#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>

class DropItemMessage : public Message
{
public:
    explicit DropItemMessage(uint32_t itemId);

    uint32_t getItemId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t itemId;
};
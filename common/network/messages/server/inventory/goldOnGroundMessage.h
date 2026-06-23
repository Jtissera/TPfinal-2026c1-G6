#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class GoldOnGroundMessage : public Message
{
public:
    GoldOnGroundMessage(uint32_t instanceId, uint32_t amount, int x, int y);

    uint32_t getInstanceId() const;
    uint32_t getAmount() const;
    int getX() const;
    int getY() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t instanceId;
    uint32_t amount;
    int x;
    int y;
};
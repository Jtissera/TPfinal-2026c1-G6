#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class NpcMoveMessage : public Message
{
public:
    NpcMoveMessage(uint32_t npcId, uint16_t x, uint16_t y);

    uint32_t getNpcId() const;
    uint16_t getX() const;
    uint16_t getY() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t npcId;
    uint16_t x;
    uint16_t y;
};
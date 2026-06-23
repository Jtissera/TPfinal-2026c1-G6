#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>

class AttackMessage : public Message
{
public:
    explicit AttackMessage(uint32_t targetId);

    uint32_t getTargetId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t targetId;
};
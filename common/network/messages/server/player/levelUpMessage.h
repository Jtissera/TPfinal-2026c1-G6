#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class LevelUpMessage : public Message
{
public:
    LevelUpMessage(uint32_t playerId, uint8_t level);

    uint32_t getPlayerId() const;
    uint8_t getLevel() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t playerId;
    uint8_t newLevel;
};
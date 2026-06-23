#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class PlayerHealthMessage : public Message
{
public:
    PlayerHealthMessage(uint32_t playerId, uint16_t hp, uint16_t hpMax);

    uint32_t getPlayerId() const;
    uint16_t getHp() const;
    uint16_t getHpMax() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t playerId;
    uint16_t hp;
    uint16_t hpMax;
};
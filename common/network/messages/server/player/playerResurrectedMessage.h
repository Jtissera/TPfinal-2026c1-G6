#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class PlayerResurrectedMessage : public Message
{
public:
    PlayerResurrectedMessage(uint32_t playerId, uint16_t tileX, uint16_t tileY);

    uint32_t getPlayerId() const;
    uint16_t getTileX() const;
    uint16_t getTileY() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t playerId;
    uint16_t tileX;
    uint16_t tileY;
};
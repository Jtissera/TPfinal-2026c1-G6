#pragma once

#include <cstdint>
#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/network/protocol/packetWriter.h"

class PlayerResurrectedMessage : public Message {
private:
    uint32_t playerId;
    uint16_t tileX;
    uint16_t tileY;

public:
    PlayerResurrectedMessage(uint32_t playerId, uint16_t tileX, uint16_t tileY)
        : playerId(playerId), tileX(tileX), tileY(tileY) {}

    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_RESURRECTED);
    }

    void serializeBody(PacketWriter& writer) const override {
        writer.writeUint32(playerId);
        writer.writeUint16(tileX);
        writer.writeUint16(tileY);
    }

    uint32_t getPlayerId() const { return playerId; }
    uint16_t getTileX() const { return tileX; }
    uint16_t getTileY() const { return tileY; }
};
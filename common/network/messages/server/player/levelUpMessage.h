#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"

class LevelUpMessage : public Message {
    uint8_t newLevel;
public:
    explicit LevelUpMessage(uint8_t level) : newLevel(level) {}
    uint8_t newLevel_get() const { return newLevel; }
    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_LEVEL_UP);
    }
    void serializeBody(PacketWriter& writer) const override {
        writer.writeUint8(newLevel);
    }
};
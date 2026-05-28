// AttackMessage.h
#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"

class AttackMessage : public Message {
    uint32_t targetId;
public:
    explicit AttackMessage(uint32_t targetId) : targetId(targetId) {}
    uint32_t getTargetId() const { return targetId; }
    uint8_t opCode() const override {
        return static_cast<uint8_t>(ClientOpCode::MSG_ATTACK);
    }

    void serializeBody(PacketWriter& writer) const override{
    writer.writeUint32(targetId);
}
};
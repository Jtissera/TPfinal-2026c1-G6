#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"

class PlayerDiedMessage : public Message {
    uint32_t id;
public:
    explicit PlayerDiedMessage(uint32_t id) : id(id) {}
    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED);
    }

    void serializeBody(PacketWriter& writer) const override {
    writer.writeUint32(id);
}
};
#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include <string>

class CombatLogMessage : public Message {
    std::string text;
public:
    explicit CombatLogMessage(std::string text) : text(std::move(text)) {}
    const std::string& getText() const { return text; }
    uint8_t opCode() const override {
        return static_cast<uint8_t>(ServerOpCode::MSG_COMBAT_LOG);
    }

    void serializeBody(PacketWriter& writer) const override{
    writer.writeString(text);
}
};
#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include <string>
#include <cstdint>

class ChatMessage : public Message
{
public:
    explicit ChatMessage(std::string text, uint32_t targetId = 0)
        : text(std::move(text)), targetId(targetId) {}

    const std::string &getText() const { return text; }
    uint32_t getTargetId() const { return targetId; }

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ClientOpCode::MSG_CHAT);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeString(text);
        writer.writeUint32(targetId);
    }

private:
    std::string text;
    uint32_t targetId;
};
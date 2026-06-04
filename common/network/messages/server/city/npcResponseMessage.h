#pragma once
#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include <string>

class NpcResponseMessage : public Message
{
public:
    explicit NpcResponseMessage(std::string text)
        : text(std::move(text)) {}

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ServerOpCode::MSG_NPC_RESPONSE);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeString(text);
    }

    const std::string &getText() const { return text; }

private:
    std::string text;
};
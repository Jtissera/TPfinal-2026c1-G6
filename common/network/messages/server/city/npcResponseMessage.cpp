#include "npcResponseMessage.h"

NpcResponseMessage::NpcResponseMessage(std::string text)
    : text(std::move(text))
{
}

const std::string &NpcResponseMessage::getText() const
{
    return text;
}

uint8_t NpcResponseMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_RESPONSE);
}

void NpcResponseMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(text);
}
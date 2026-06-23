#include "combatLogMessage.h"

CombatLogMessage::CombatLogMessage(std::string text)
    : text(std::move(text))
{
}

const std::string &CombatLogMessage::getText() const
{
    return text;
}

uint8_t CombatLogMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_COMBAT_LOG);
}

void CombatLogMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(text);
}
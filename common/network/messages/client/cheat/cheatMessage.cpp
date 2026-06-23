#include "cheatMessage.h"

CheatMessage::CheatMessage(CheatType cheat)
    : cheat(cheat)
{
}

CheatType CheatMessage::getCheat() const
{
    return cheat;
}

uint8_t CheatMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CHEAT);
}

void CheatMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint8(static_cast<uint8_t>(cheat));
}
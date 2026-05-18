#include "listGamesMessage.h"

uint8_t ListGamesMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES);
}

void ListGamesMessage::serializeBody(PacketWriter &) const {}
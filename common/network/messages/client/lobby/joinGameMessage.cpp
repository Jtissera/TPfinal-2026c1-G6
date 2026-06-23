#include "joinGameMessage.h"

JoinGameMessage::JoinGameMessage(uint32_t gameId)
    : gameId(gameId)
{
}

uint32_t JoinGameMessage::getGameId() const
{
    return gameId;
}

uint8_t JoinGameMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME);
}

void JoinGameMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(gameId);
}
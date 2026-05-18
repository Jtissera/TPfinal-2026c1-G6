#include "joinOkMessage.h"

JoinOkMessage::JoinOkMessage(uint32_t gameId, std::string gameName)
    : gameId(gameId), gameName(std::move(gameName)) {}

uint32_t JoinOkMessage::getGameId() const
{
    return gameId;
}

const std::string &JoinOkMessage::getGameName() const
{
    return gameName;
}

uint8_t JoinOkMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK);
}

void JoinOkMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(gameId);
    writer.writeString(gameName);
}
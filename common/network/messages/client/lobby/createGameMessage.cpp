#include "createGameMessage.h"

CreateGameMessage::CreateGameMessage(std::string gameName, uint8_t maxPlayers)
    : gameName(std::move(gameName)), maxPlayers(maxPlayers) {}

const std::string &CreateGameMessage::getGameName() const
{
    return gameName;
}

uint8_t CreateGameMessage::getMaxPlayers() const
{
    return maxPlayers;
}

uint8_t CreateGameMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME);
}

void CreateGameMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(gameName);
    writer.writeUint8(maxPlayers);
}
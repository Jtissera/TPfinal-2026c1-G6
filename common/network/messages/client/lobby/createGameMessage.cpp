#include "createGameMessage.h"

CreateGameMessage::CreateGameMessage(std::string gameName,
                                     uint8_t maxPlayers,
                                     std::string mapPath)
    : gameName(std::move(gameName)),
      maxPlayers(maxPlayers),
      mapPath(std::move(mapPath))
{
}

const std::string &CreateGameMessage::getGameName() const { return gameName; }
uint8_t CreateGameMessage::getMaxPlayers() const { return maxPlayers; }
const std::string &CreateGameMessage::getMapPath() const { return mapPath; }

uint8_t CreateGameMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME);
}

void CreateGameMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(gameName);
    writer.writeUint8(maxPlayers);
    writer.writeString(mapPath);
}
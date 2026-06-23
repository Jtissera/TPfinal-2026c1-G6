#include "gameCreatedMessage.h"

GameCreatedMessage::GameCreatedMessage(uint32_t gameId,
                                       std::string gameName,
                                       uint8_t maxPlayers)
    : gameId(gameId),
      gameName(std::move(gameName)),
      maxPlayers(maxPlayers)
{
}

uint32_t GameCreatedMessage::getGameId() const { return gameId; }
const std::string &GameCreatedMessage::getGameName() const { return gameName; }
uint8_t GameCreatedMessage::getMaxPlayers() const { return maxPlayers; }

uint8_t GameCreatedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED);
}

void GameCreatedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(gameId);
    writer.writeString(gameName);
    writer.writeUint8(maxPlayers);
}
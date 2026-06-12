#include "gameListMessage.h"

GameListMessage::GameListMessage(std::vector<GameInfo> games)
    : games(std::move(games)) {}

const std::vector<GameInfo> &GameListMessage::getGames() const
{
    return games;
}

uint8_t GameListMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST);
}

void GameListMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint8(static_cast<uint8_t>(games.size()));
    for (const auto &game : games)
    {
        writer.writeUint32(game.gameId);
        writer.writeString(game.gameName);
        writer.writeUint8(game.playerCount);
        writer.writeUint8(game.maxPlayers);
        writer.writeString(game.mapPath);
    }
}
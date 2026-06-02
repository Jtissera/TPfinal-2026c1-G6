#include "lobbyServerDeserializersModule.h"

#include "common/network/messages/server/lobby/gameListMessage.h"
#include "common/network/messages/server/lobby/gameCreatedMessage.h"
#include "common/network/messages/server/lobby/joinOkMessage.h"
#include "common/network/messages/server/lobby/leaveOkMessage.h"
#include "common/network/protocol/serverOpCode.h"

void LobbyServerDeserializersModule::registerDeserializers(Registry& registry) const {

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            uint8_t count = reader.readUint8();
            std::vector<GameInfo> games;
            games.reserve(count);
            for (uint8_t i = 0; i < count; ++i) {
                GameInfo info;
                info.gameId      = reader.readUint32();
                info.gameName    = reader.readString();
                info.playerCount = reader.readUint8();
                info.maxPlayers  = reader.readUint8();
                games.push_back(std::move(info));
            }
            return std::make_unique<GameListMessage>(std::move(games));
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto gameId     = reader.readUint32();
            auto gameName   = reader.readString();
            auto maxPlayers = reader.readUint8();
            return std::make_unique<GameCreatedMessage>(
                gameId, std::move(gameName), maxPlayers);
        });

registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        auto gameId   = reader.readUint32();
        auto gameName = reader.readString();
        auto spawnX   = reader.readUint16();
        auto spawnY   = reader.readUint16();
        return std::make_unique<JoinOkMessage>(gameId, std::move(gameName), spawnX, spawnY);
    });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_LEAVE_OK),
        [](PacketReader&) -> std::unique_ptr<Message> {
            return std::make_unique<LeaveOkMessage>();
        });
}
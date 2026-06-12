#include "lobbyClientDeserializersModule.h"

void LobbyClientDeserializersModule::registerDeserializers(
    Registry &registry) const
{
  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES),
      [](PacketReader &) -> std::unique_ptr<Message>
      {
        return std::make_unique<ListGamesMessage>();
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        auto gameName = reader.readString();
        auto maxPlayers = reader.readUint8();
        auto mapPath = reader.readString();
        return std::make_unique<CreateGameMessage>(std::move(gameName),
                                                   maxPlayers, std::move(mapPath));
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        auto gameId = reader.readUint32();
        return std::make_unique<JoinGameMessage>(gameId);
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_LEAVE_GAME),
      [](PacketReader &) -> std::unique_ptr<Message>
      {
        return std::make_unique<LeaveGameMessage>();
      });
}
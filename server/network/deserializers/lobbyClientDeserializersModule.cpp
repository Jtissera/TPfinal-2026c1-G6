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
        std::string gameName = reader.readString();
        uint8_t maxPlayers = reader.readUint8();
        std::string mapPath = reader.readString();
        return std::make_unique<CreateGameMessage>(
            std::move(gameName), maxPlayers, std::move(mapPath));
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        uint32_t gameId = reader.readUint32();
        return std::make_unique<JoinGameMessage>(gameId);
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_LEAVE_GAME),
      [](PacketReader &) -> std::unique_ptr<Message>
      {
        return std::make_unique<LeaveGameMessage>();
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_LOGIN),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        std::string characterName = reader.readString();
        return std::make_unique<LoginMessage>(std::move(characterName));
      });
}
#include "lobbyServerDeserializersModule.h"

#include <iostream>

#include "../../../../common/network/messages/server/auth/connectOKMessage.h"
#include "../../../../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../../../../common/network/messages/server/lobby/gameListMessage.h"
#include "../../../../common/network/messages/server/lobby/joinOkMessage.h"
#include "../../../../common/network/protocol/serverOpCode.h"

void LobbyServerDeserializersModule::registerDeserializers(
    Registry &registry) const
{
  registry.registerDeserializer(
      static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        uint8_t count = reader.readUint8();
        std::vector<GameInfo> games;
        games.reserve(count);
        for (uint8_t i = 0; i < count; ++i)
        {
          GameInfo info;
          info.gameId = reader.readUint32();
          info.gameName = reader.readString();
          info.playerCount = reader.readUint8();
          info.maxPlayers = reader.readUint8();
          info.mapPath = reader.readString();
          std::cout << "[NET DEBUG] Recibido mapa del server: '" << info.mapPath << "'" << std::endl;
          games.push_back(std::move(info));
        }
        return std::make_unique<GameListMessage>(std::move(games));
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        auto gameId = reader.readUint32();
        auto gameName = reader.readString();
        auto maxPlayers = reader.readUint8();
        return std::make_unique<GameCreatedMessage>(gameId, std::move(gameName),
                                                    maxPlayers);
      });

  registry.registerDeserializer(
      static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        auto gameId = reader.readUint32();
        auto gameName = reader.readString();

        PlayerDto playerDto{};

        playerDto.nombre = reader.readString();
        playerDto.playerID = reader.readUint8();
        playerDto.raza = reader.readString();
        playerDto.clase = reader.readString();
        playerDto.headId = static_cast<int>(reader.readUint32());
        playerDto.level = reader.readUint8();

        playerDto.hp = static_cast<int>(reader.readUint16());
        playerDto.mana = static_cast<int>(reader.readUint16());
        playerDto.hpMax = static_cast<int>(reader.readUint16());
        playerDto.manaMax = static_cast<int>(reader.readUint16());

        playerDto.oro = static_cast<int>(reader.readUint32());
        playerDto.oroMax = static_cast<int>(reader.readUint32());

        playerDto.xpos = reader.readUint16();
        playerDto.ypos = reader.readUint16();

        playerDto.exp = static_cast<int>(reader.readUint32());
        playerDto.expMax = static_cast<int>(reader.readUint32());

        playerDto.esFantasma = reader.readUint8() != 0;

        playerDto.fuerza = static_cast<int>(reader.readUint32());
        playerDto.agilidad = static_cast<int>(reader.readUint32());
        playerDto.inteligencia = static_cast<int>(reader.readUint32());
        playerDto.constitucion = static_cast<int>(reader.readUint32());

        return std::make_unique<JoinOkMessage>(gameId, std::move(gameName),
                                               std::move(playerDto));
      });
}

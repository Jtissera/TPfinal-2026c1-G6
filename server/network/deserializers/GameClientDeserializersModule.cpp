
#include "GameClientDeserializersModule.h"

#include "common/dtos/gameTypes.h"
#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/protocol/clientOpCode.h"

void GameClientDeserializersModule::registerDeserializers(
    Registry &registry) const {
  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_MOVE),
      [](PacketReader &reader) -> std::unique_ptr<Message> {
        auto dir = static_cast<Direction>(reader.readUint8());
        return std::make_unique<MoveMessage>(dir);
      });
  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_CHEAT),
      [](PacketReader &reader) -> std::unique_ptr<Message> {
        auto cheat = static_cast<CheatType>(reader.readUint8());
        return std::make_unique<CheatMessage>(cheat);
      });
}

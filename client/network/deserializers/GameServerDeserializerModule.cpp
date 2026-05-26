#include "GameServerDeserializerModule.h"
#include "common/network/messages/server/player/playerStatsMessage.h"

void GameServerDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto id = reader.readUint8();
            auto x  = reader.readUint16();
            auto y  = reader.readUint16();
            return std::make_unique<EntityMoveMessage>(id, x, y);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_STATS),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto level    = reader.readUint8();
            auto hp       = static_cast<int16_t>(reader.readUint16());
            auto maxHp    = static_cast<int16_t>(reader.readUint16());
            auto mana     = static_cast<int16_t>(reader.readUint16());
            auto maxMana  = static_cast<int16_t>(reader.readUint16());
            auto exp      = reader.readUint32();
            auto expLimit = reader.readUint32();
            auto gold     = reader.readUint32();
            return std::make_unique<PlayerStatsMessage>(
                level, hp, maxHp, mana, maxMana, exp, expLimit, gold);
        });
}
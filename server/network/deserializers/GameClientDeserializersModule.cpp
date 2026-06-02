
#include "GameClientDeserializersModule.h"

#include "common/dtos/gameTypes.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/protocol/clientOpCode.h"


void GameClientDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto dir = static_cast<Direction>(reader.readUint8());
            return std::make_unique<MoveMessage>(dir);
        });

    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_ENEMY_HIT_PLAYER),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        uint32_t enemyId = reader.readUint32();
        return std::make_unique<EnemyHitPlayerMessage>(enemyId);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        (void)reader;
        return std::make_unique<ResurrectMessage>();
    });
}
